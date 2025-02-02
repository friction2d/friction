/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "canvaswrappernode.h"
#include "widgets/scenechooser.h"
#include "Private/document.h"

class CanvasWrapperMenuBar : public StackWrapperMenu {
public:
    CanvasWrapperMenuBar(Document& document, CanvasWindow * const window) :
        mDocument(document), mWindow(window) {
        mSceneMenu = new SceneChooser(mDocument, false, this);
        addMenu(mSceneMenu);
        connect(mSceneMenu, &SceneChooser::currentChanged,
                this, &CanvasWrapperMenuBar::setCurrentScene);
        connect(window, &CanvasWindow::currentSceneChanged,
                mSceneMenu, qOverload<Canvas*>(&SceneChooser::setCurrentScene));
    }

    void setCurrentScene(Canvas * const scene) {
        mWindow->setCurrentCanvas(scene);
        mSceneMenu->setCurrentScene(scene);
    }

    Canvas* getCurrentScene() const { return mCurrentScene; }
private:
    Document& mDocument;
    CanvasWindow* const mWindow;
    SceneChooser * mSceneMenu;
    Canvas * mCurrentScene = nullptr;
    std::map<Canvas*, QAction*> mSceneToAct;
};

CanvasWrapperNode::CanvasWrapperNode(Canvas* const scene) :
    WidgetWrapperNode([](Canvas* const scene) {
        return new CanvasWrapperNode(scene);
    }) {
    mCanvasWindow = new CanvasWindow(*Document::sInstance, this);
    mMenu = new CanvasWrapperMenuBar(*Document::sInstance, mCanvasWindow);
    setMenuBar(mMenu);
    setCentralWidget(mCanvasWindow);
    mMenu->setCurrentScene(scene);
}

void CanvasWrapperNode::readData(eReadStream &src) {
    mCanvasWindow->readState(src);
    mMenu->setCurrentScene(mCanvasWindow->getCurrentCanvas());
}

void CanvasWrapperNode::writeData(eWriteStream &dst) {
    mCanvasWindow->writeState(dst);
}

void CanvasWrapperNode::readDataXML(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                                    const QDomElement& ele,
                                    Friction::Core::RuntimeIdToWriteId& objListIdConv) {
    Q_UNUSED(objListIdConv)
    mCanvasWindow->readStateXML(boxReadHandler, ele);
    mMenu->setCurrentScene(mCanvasWindow->getCurrentCanvas());
}

void CanvasWrapperNode::writeDataXML(QDomElement& ele, QDomDocument& doc,
                                     Friction::Core::RuntimeIdToWriteId& objListIdConv) {
    Q_UNUSED(objListIdConv)
    mCanvasWindow->writeStateXML(ele, doc);
}
