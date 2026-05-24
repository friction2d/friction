#include "oilsimulator.h"
#include "oiltrace.h"

#include "skia/skiahelpers.h"
#include "simplemath.h"

#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcOil)

unsigned int OilSimulator::MAX_INVALID_TRAJECTORIES = 5000;

unsigned int OilSimulator::MAX_INVALID_TRAJECTORIES_FOR_SMALLER_SIZE = 10000;

unsigned int OilSimulator::MAX_INVALID_TRACES = 250;

unsigned int OilSimulator::MAX_INVALID_TRACES_FOR_SMALLER_SIZE = 350;

float OilSimulator::TRACE_SPEED = 2;

SkColor OilSimulator::BACKGROUND_COLOR = SK_ColorTRANSPARENT;

float OilSimulator::MAX_VISITS_FRACTION_IN_TRAJECTORY = 0.35;

float OilSimulator::MIN_INSIDE_FRACTION_IN_TRAJECTORY = 0.4;

float OilSimulator::MAX_SIMILAR_COLOR_FRACTION_IN_TRAJECTORY = 0.6;

float OilSimulator::MAX_COLOR_STDEV_IN_TRAJECTORY = 45;

float OilSimulator::MIN_INSIDE_FRACTION = 0.7;

float OilSimulator::MAX_SIMILAR_COLOR_FRACTION = 0.8; // 0.8 - 0.85 - 0.5

float OilSimulator::MAX_PAINTED_FRACTION = 0.65;

float OilSimulator::MIN_COLOR_IMPROVEMENT_FACTOR = 0.6;

float OilSimulator::BIG_WELL_PAINTED_IMPROVEMENT_FRACTION = 0.3; // 0.3 - 0.35 - 0.4

float OilSimulator::MIN_BAD_PAINTED_REDUCTION_FRACTION = 0.45; // 0.45 - 0.3 - 0.45

float OilSimulator::MAX_WELL_PAINTED_DESTRUCTION_FRACTION = 0.4; // 0.4 - 0.55 - 0.4

OilSimulator::OilSimulator(const bool _useGpu,
                           bool _useCanvasBuffer,
                           bool _verbose) :
    mUseGpu(_useGpu),
    useCanvasBuffer(_useCanvasBuffer),
    verbose(_verbose) {
    nBadPaintedPixels = 0;
    averageBrushSize = SMALLER_BRUSH_SIZE;
    paintingIsFinised = true;
    obtainNewTrace = false;
    traceStep = 0;
    nTraces = 0;
}

OilSimulator::OilSimulator(SkBitmap& dst, bool _useCanvasBuffer, bool _verbose) :
        OilSimulator(false, _useCanvasBuffer, _verbose) {
    mCpuDst = dst;
}

OilSimulator::OilSimulator(SkCanvas& dst, bool _useCanvasBuffer, bool _verbose) :
        OilSimulator(true, _useCanvasBuffer, _verbose) {
    mGpuDst = &dst;
    mCanvas = std::shared_ptr<SkCanvas>(&dst, [](SkCanvas*){});
}

void OilSimulator::setImage(const SkBitmap& imagePixels, bool clearCanvas) {
	// Set the image pixels
    mImg = imagePixels;
    int imgWidth = mImg.width();
    int imgHeight = mImg.height();

	// Initialize the canvas and pixel containers if necessary
    if (clearCanvas || imgWidth != mCanvasWidth || imgHeight != mCanvasHeight) {
        const auto imgInfo = SkiaHelpers::getPremulRGBAInfo(imgWidth, imgHeight);

		// Initialize the canvas where the image will be painted
        if(mUseGpu) mCpuDst.allocPixels(imgInfo);
        else mCanvas = std::make_shared<SkCanvas>(mCpuDst);
        mCanvas->clear(BACKGROUND_COLOR);
        mCanvasWidth = imgWidth;
        mCanvasHeight = imgHeight;

		// Initialize the canvas buffer if necessary
        if (useCanvasBuffer) {
            mPaintedPixels.allocPixels(imgInfo);

            SkBitmap canvasBuffer;
            canvasBuffer.allocPixels(imgInfo);
            mCanvasBuffer = std::make_shared<SkCanvas>(canvasBuffer);
            mCanvasBuffer->clear(BACKGROUND_COLOR);
		}

		// Initialize all the pixel arrays
        mVisitedPixels.resize(imgWidth*imgHeight);
		badPaintedPixels = vector<unsigned int>(imgWidth * imgHeight);
		nBadPaintedPixels = 0;
	}

	// Initialize the rest of the simulator variables
    averageBrushSize = qMax(SMALLER_BRUSH_SIZE, BIGGER_BRUSH_SIZE);
	paintingIsFinised = false;
	obtainNewTrace = true;
	traceStep = 0;
	nTraces = 0;
}

void OilSimulator::update(bool stepByStep) {
	// Don't do anything if the painting is finished
	if (paintingIsFinised) {
		return;
	}

	// Check if a new trace should be obtained
	if (obtainNewTrace) {
		// Update the pixel arrays
		updatePixelArrays();

		// Get a new trace
		getNewTrace();
	}

	// Paint the current trace if the painting is not finished
	if (!paintingIsFinised) {
		if (stepByStep) {
			// Paint the current trace step
			paintTraceStep();

			// Check if we finished painting the trace
			if (traceStep == trace.getNSteps()) {
				obtainNewTrace = true;
			}
		} else {
			// Paint all the trace steps
			paintTrace();
			obtainNewTrace = true;
		}
	}
}

void OilSimulator::updatePixelArrays() {
	// Update the visited pixels array
	updateVisitedPixels();

    if(mUseGpu) {
        mGpuDst->flush();
        const bool ret = mGpuDst->readPixels(mCpuDst, 0, 0);
        if(!ret) RuntimeThrow("Could not read gpu canvas pixels");
    }

	// Update the painted pixels array
	if (useCanvasBuffer) {
        const bool ret = mCanvasBuffer->readPixels(mPaintedPixels, 0, 0);
        if(!ret) RuntimeThrow("Could not read canvas buffer pixels");
    }

	// Update the similar color pixels and the bad painted pixels arrays
    const auto imgPixels = static_cast<uchar*>(mImg.getAddr(0, 0));
    const auto paintedPixels = static_cast<uchar*>(useCanvasBuffer ?
                                                   mPaintedPixels.getAddr(0, 0) :
                                                   mCpuDst.getAddr(0, 0));

    unsigned int imgNumChannels = 4;
    unsigned int canvasNumChannels = 4;
	nBadPaintedPixels = 0;

    const int bgRed = SkColorGetR(BACKGROUND_COLOR);
    const int bgGreen = SkColorGetG(BACKGROUND_COLOR);
    const int bgBlue = SkColorGetB(BACKGROUND_COLOR);

    for (unsigned int pixel = 0, nPixels = mImg.width() * mImg.height(); pixel < nPixels; ++pixel) {
		unsigned int imgPix = pixel * imgNumChannels;
		unsigned int canvasPix = pixel * canvasNumChannels;

		// Check if the pixel is well painted
        if (paintedPixels[canvasPix] != bgRed && paintedPixels[canvasPix + 1] != bgGreen
                && paintedPixels[canvasPix + 2] != bgBlue
                && abs(imgPixels[imgPix] - paintedPixels[canvasPix]) < MAX_COLOR_DIFFERENCE[0]
                && abs(imgPixels[imgPix + 1] - paintedPixels[canvasPix + 1]) < MAX_COLOR_DIFFERENCE[1]
                && abs(imgPixels[imgPix + 2] - paintedPixels[canvasPix + 2]) < MAX_COLOR_DIFFERENCE[2]) {
		} else {
			badPaintedPixels[nBadPaintedPixels] = pixel;
			++nBadPaintedPixels;
		}
	}
}

void OilSimulator::updateVisitedPixels() {
	// Check if we are at the beginning of a simulation
	if (nTraces == 0) {
		// Reset the visited pixels array
        for(auto& pix : mVisitedPixels) pix = 255;
	} else {
		// Update the visited pixels arrays with the trace bristle positions
		const vector<unsigned char>& alphas = trace.getTrajectoryAphas();
        const vector<vector<SkPoint>>& bristlePositions = trace.getBristlePositions();
        int width = mImg.width();
        int height = mImg.height();

		for (unsigned int i = 0, nSteps = trace.getNSteps(); i < nSteps; ++i) {
			// Fill the visited pixels array if alpha is high enough
            if (alphas[i] >= OilTrace::MIN_ALPHA) {
				for (const SkPoint& pos : bristlePositions[i]) {
                    int x = pos.x();
                    int y = pos.y();

					if (x >= 0 && x < width && y >= 0 && y < height) {
                        mVisitedPixels.at(y*width + x) = 0;
					}
				}
			}
		}
	}
}

void OilSimulator::getNewTrace() {
	// Loop until a new trace is found or the painting is finished
	unsigned int invalidTrajectoriesCounter = 0;
	unsigned int invalidTracesCounter = 0;
    int imgWidth = mImg.width();

	while (true) {
		// Check if we should stop the painting simulation
		if (averageBrushSize == SMALLER_BRUSH_SIZE
				&& (invalidTrajectoriesCounter > MAX_INVALID_TRAJECTORIES_FOR_SMALLER_SIZE
						|| invalidTracesCounter > MAX_INVALID_TRACES_FOR_SMALLER_SIZE)) {
			// Print some debug information if necessary
            if (verbose) {
                qCDebug(lcOil) << "Total number of painted traces: " << nTraces;
            }

			// Stop the painting
			paintingIsFinised = true;
			break;
		} else {
			// Change the average brush size if there were too many invalid traces
			if (averageBrushSize > SMALLER_BRUSH_SIZE
					&& (invalidTrajectoriesCounter > MAX_INVALID_TRAJECTORIES
							|| invalidTracesCounter > MAX_INVALID_TRACES)) {
				// Decrease the brush size
                averageBrushSize = qMax(SMALLER_BRUSH_SIZE,
                        qMin(averageBrushSize / BRUSH_SIZE_DECREMENT, averageBrushSize - 2));

				// Print some debug information if necessary
                if (verbose) {
                    qCDebug(lcOil) << "traces = " << nTraces << ", new average brush size = " << averageBrushSize << "";
                }

				// Reset some the counters
				invalidTrajectoriesCounter = 0;
				invalidTracesCounter = 0;

				// Reset the visited pixels array
                for(auto& pix : mVisitedPixels) pix = 255;
			}

			// Create new traces until one of them has a valid trajectory or we exceed a number of tries
			bool isValidTrajectory = false;
            float brushSize = qMax(SMALLER_BRUSH_SIZE, averageBrushSize * gSkRandF(0.95, 1.05));
            int nSteps = qMax(MIN_TRACE_LENGTH, RELATIVE_TRACE_LENGTH * brushSize * gSkRandF(0.9, 1.1)) / TRACE_SPEED;

			while (!isValidTrajectory && invalidTrajectoriesCounter % 500 != 499) {
				// Create the trace starting from a bad painted pixel
                unsigned int pixel = badPaintedPixels[floor(gSkRandF(0, nBadPaintedPixels))];
                SkPoint startingPosition = SkPoint::Make(pixel % imgWidth, pixel / imgWidth);
                trace = OilTrace(startingPosition, nSteps, TRACE_SPEED);

				// Check if the trace has a valid trajectory
				isValidTrajectory = !alreadyVisitedTrajectory() && validTrajectory();

				// Increase the counter
				++invalidTrajectoriesCounter;
			}

			// Check if we have a valid trajectory
			if (isValidTrajectory) {
				// Reset the invalid trajectories counter
				invalidTrajectoriesCounter = 0;

				// Set the trace brush size
                trace.setBrushSize(brushSize, BRISTLE_THICKNESS, BRISTLE_DENSITY);

				// Calculate the trace average color and the bristle colors along the trajectory
                trace.calculateAverageColor(mImg);
                trace.calculateBristleColors(useCanvasBuffer ? mPaintedPixels : mCpuDst,
                                             BACKGROUND_COLOR);

				// Check if painting the trace will improve the painting
				if (traceImprovesPainting()) {
					// Test passed, the trace is good enough to be painted
					obtainNewTrace = false;
					traceStep = 0;
					++nTraces;
					break;
				} else {
					// The trace is not good enough, try again in the next loop step
					++invalidTracesCounter;
				}
			} else {
				// The trace is not good enough, try again in the next loop step
				++invalidTracesCounter;
			}
		}
	}
}

bool OilSimulator::alreadyVisitedTrajectory() const {
	// Extract some useful information
    const vector<SkPoint>& positions = trace.getTrajectoryPositions();
	const vector<unsigned char>& alphas = trace.getTrajectoryAphas();
    int width = mImg.width();
    int height = mImg.height();

	// Check if the trace trajectory has been visited before
	int insideCounter = 0;
	int visitedCounter = 0;

    for (unsigned int i = OilBrush::POSITIONS_FOR_AVERAGE, nSteps = trace.getNSteps(); i < nSteps; ++i) {
		// Check that the alpha value is high enough
        if (alphas[i] >= OilTrace::MIN_ALPHA) {
			// Check that the position is inside the image
			const SkPoint& pos = positions[i];
            int x = pos.x();
            int y = pos.y();

			if (x >= 0 && x < width && y >= 0 && y < height) {
				++insideCounter;

                if (mVisitedPixels.at(y*width + x) == 0) {
					++visitedCounter;
				}
			}
		}
	}

	return visitedCounter > MAX_VISITS_FRACTION_IN_TRAJECTORY * insideCounter;
}

bool OilSimulator::validTrajectory() const {
	// Extract some useful information
    const vector<SkPoint>& positions = trace.getTrajectoryPositions();
	const vector<unsigned char>& alphas = trace.getTrajectoryAphas();
    int width = mImg.width();
    int height = mImg.height();

	// Obtain some pixel statistics along the trajectory
	int insideCounter = 0;
	int outsideCounter = 0;
	int similarColorCounter = 0;
	float imgRedSum = 0;
	float imgRedSqSum = 0;
	float imgGreenSum = 0;
	float imgGreenSqSum = 0;
	float imgBlueSum = 0;
	float imgBlueSqSum = 0;

    for (unsigned int i = OilBrush::POSITIONS_FOR_AVERAGE, nSteps = trace.getNSteps(); i < nSteps; ++i) {
		// Check that the alpha value is high enough
        if (alphas[i] >= OilTrace::MIN_ALPHA) {
			// Check that the position is inside the image
			const SkPoint& pos = positions[i];
            int x = pos.x();
            int y = pos.y();

			if (x >= 0 && x < width && y >= 0 && y < height) {
				++insideCounter;

				// Get the image color and the painted color at the trajectory position
                const SkColor imgColor = mImg.getColor(x, y);
                const SkColor paintedColor = useCanvasBuffer ?
                                     mPaintedPixels.getColor(x, y) :
                                     mCpuDst.getColor(x, y);

                // Extract the pixel color properties
                const int imgRed = SkColorGetR(imgColor);
                const int imgGreen = SkColorGetG(imgColor);
                const int imgBlue = SkColorGetB(imgColor);

                const int paintedRed = SkColorGetR(paintedColor);
                const int paintedGreen = SkColorGetG(paintedColor);
                const int paintedBlue = SkColorGetB(paintedColor);

				// Check if the two colors are similar
                if (paintedColor != BACKGROUND_COLOR && abs(imgRed - paintedRed) < MAX_COLOR_DIFFERENCE[0]
                        && abs(imgGreen - paintedGreen) < MAX_COLOR_DIFFERENCE[1]
                        && abs(imgBlue - paintedBlue) < MAX_COLOR_DIFFERENCE[2]) {
					++similarColorCounter;
				}

				imgRedSum += imgRed;
				imgRedSqSum += imgRed * imgRed;
				imgGreenSum += imgGreen;
				imgGreenSqSum += imgGreen * imgGreen;
				imgBlueSum += imgBlue;
				imgBlueSqSum += imgBlue * imgBlue;
			} else {
				++outsideCounter;
			}
		}
	}

	// Obtain the image colors standard deviation along the trajectory
	float imgRedStDevSq = 0;
	float imgGreenStDevSq = 0;
	float imgBlueStDevSq = 0;

	if (insideCounter > 1) {
		imgRedStDevSq = (imgRedSqSum - imgRedSum * imgRedSum / insideCounter) / (insideCounter - 1);
		imgGreenStDevSq = (imgGreenSqSum - imgGreenSum * imgGreenSum / insideCounter) / (insideCounter - 1);
		imgBlueStDevSq = (imgBlueSqSum - imgBlueSum * imgBlueSum / insideCounter) / (insideCounter - 1);
	}

	// Check if we have a valid trajectory
	bool insideCanvas = insideCounter >= MIN_INSIDE_FRACTION_IN_TRAJECTORY * (insideCounter + outsideCounter);
	bool badPainted = similarColorCounter <= MAX_SIMILAR_COLOR_FRACTION_IN_TRAJECTORY * insideCounter;
	float maxSqDevSq = pow(MAX_COLOR_STDEV_IN_TRAJECTORY, 2);
	bool smallColorChange = imgRedStDevSq < maxSqDevSq && imgGreenStDevSq < maxSqDevSq && imgBlueStDevSq < maxSqDevSq;

	return insideCanvas && badPainted && smallColorChange;
}

bool OilSimulator::traceImprovesPainting() const {
	// Extract some useful information
	const vector<unsigned char>& alphas = trace.getTrajectoryAphas();
    const vector<vector<SkColor>>& bristleImgColors = trace.getBristleImageColors();
    const vector<vector<SkColor>>& bristlePaintedColors = trace.getBristlePaintedColors();
    const vector<vector<SkColor>>& bristleColors = trace.getBristleColors();

	// Obtain some trace statistics
	int insideCounter = 0;
	int outsideCounter = 0;
	int paintedCounter = 0;
	int similarColorCounter = 0;
	int wellPaintedCounter = 0;
	int destroyedSimilarColorCounter = 0;
	int colorImprovement = 0;

	for (unsigned int i = 0, nSteps = trace.getNSteps(); i < nSteps; ++i) {
		// Check that the alpha value is high enough
        if (alphas[i] >= OilTrace::MIN_ALPHA) {
			// Get the bristles image colors and painted colors for this step
            const vector<SkColor>& bic = bristleImgColors[i];
            const vector<SkColor>& bpc = bristlePaintedColors[i];
            const vector<SkColor>& bc = bristleColors[i];

			// Make sure that the containers are not empty
			if (bic.size() > 0) {
				for (unsigned int bristle = 0, nBristles = trace.getNBristles(); bristle < nBristles; ++bristle) {
					// Get the image color and the painted color at the bristle position
                    const SkColor& imgColor = bic[bristle];
                    const SkColor& paintedColor = bpc[bristle];
                    const SkColor& bristleColor = bc[bristle];

					// Check that the bristle is inside the image
                    if (SkColorGetA(imgColor) != 0) {
						++insideCounter;

						// Count the number of painted pixels
                        bool paintedPixel = SkColorGetA(paintedColor) != 0;

						if (paintedPixel) {
							++paintedCounter;
						}

                        // Extract the pixel color properties
                        const int imgRed = SkColorGetR(imgColor);
                        const int imgGreen = SkColorGetG(imgColor);
                        const int imgBlue = SkColorGetB(imgColor);

                        const int paintedRed = SkColorGetR(paintedColor);
                        const int paintedGreen = SkColorGetG(paintedColor);
                        const int paintedBlue = SkColorGetB(paintedColor);

                        const int bristleRed = SkColorGetR(bristleColor);
                        const int bristleGreen = SkColorGetG(bristleColor);
                        const int bristleBlue = SkColorGetB(bristleColor);

						// Count the number of painted pixels whose color is similar to the image color
                        int redPaintedDiff = abs(imgRed - paintedRed);
                        int greenPaintedDiff = abs(imgGreen - paintedGreen);
                        int bluePaintedDiff = abs(imgBlue - paintedBlue);
						bool similarColorPixel = paintedPixel && redPaintedDiff < MAX_COLOR_DIFFERENCE[0]
								&& greenPaintedDiff < MAX_COLOR_DIFFERENCE[1]
								&& bluePaintedDiff < MAX_COLOR_DIFFERENCE[2];

						if (similarColorPixel) {
							++similarColorCounter;
						}

						// Count the number of pixels that will be well painted
                        int redAverageDiff = abs(imgRed - bristleRed);
                        int greenAverageDiff = abs(imgGreen - bristleGreen);
                        int blueAverageDiff = abs(imgBlue - bristleBlue);
						bool wellPaintedPixel = redAverageDiff < MAX_COLOR_DIFFERENCE[0]
								&& greenAverageDiff < MAX_COLOR_DIFFERENCE[1]
								&& blueAverageDiff < MAX_COLOR_DIFFERENCE[2];

						if (wellPaintedPixel) {
							++wellPaintedCounter;
						}

						// Count the number of pixels that will not be well painted anymore
						if (similarColorPixel && !wellPaintedPixel) {
							++destroyedSimilarColorCounter;
						}

						// Calculate the color improvement
						if (paintedPixel) {
							colorImprovement += redPaintedDiff - redAverageDiff + greenPaintedDiff - greenAverageDiff
									+ bluePaintedDiff - blueAverageDiff;
						}
					} else {
						++outsideCounter;
					}
				}
			}
		}
	}

	int wellPaintedImprovement = wellPaintedCounter - similarColorCounter;
	int previouslyBadPainted = insideCounter - similarColorCounter;
	float averageMaxColorDiff = (MAX_COLOR_DIFFERENCE[0] + MAX_COLOR_DIFFERENCE[1] + MAX_COLOR_DIFFERENCE[2]) / 3.0;

	bool outsideCanvas = insideCounter < MIN_INSIDE_FRACTION * (insideCounter + outsideCounter);
	bool alreadyWellPainted = similarColorCounter > MAX_SIMILAR_COLOR_FRACTION * insideCounter;
	bool alreadyPainted = paintedCounter >= MAX_PAINTED_FRACTION * insideCounter;
	bool colorImproves = colorImprovement >= MIN_COLOR_IMPROVEMENT_FACTOR * averageMaxColorDiff * paintedCounter;
	bool bigWellPaintedImprovement = wellPaintedImprovement >= BIG_WELL_PAINTED_IMPROVEMENT_FRACTION * insideCounter;
	bool reducedBadPainted = wellPaintedImprovement >= MIN_BAD_PAINTED_REDUCTION_FRACTION * previouslyBadPainted;
	bool lowWellPaintedDestruction = destroyedSimilarColorCounter
			<= MAX_WELL_PAINTED_DESTRUCTION_FRACTION * wellPaintedImprovement;
	bool improves = (colorImproves || bigWellPaintedImprovement) && reducedBadPainted && lowWellPaintedDestruction;

	// Check if the trace will improve the painting
	return (outsideCanvas || alreadyWellPainted || (alreadyPainted && !improves)) ? false : true;
}

void OilSimulator::paintTrace() {
	// Pain the trace in the canvas and the canvas buffer if necessary
    if(useCanvasBuffer) {
        trace.paint(*mCanvas, *mCanvasBuffer);
    } else {
        trace.paint(*mCanvas);
    }
}

void OilSimulator::paintTraceStep() {
	// Pain the trace step in the canvas and the canvas buffer if necessary
    if(useCanvasBuffer) {
        trace.paintStep(*mCanvas, traceStep, *mCanvasBuffer);
    } else {
        trace.paintStep(*mCanvas, traceStep);
    }

	// Increment the trace step
	++traceStep;
}

bool OilSimulator::isFinished() const {
	return paintingIsFinised;
}
