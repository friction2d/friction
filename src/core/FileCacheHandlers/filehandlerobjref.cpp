#include "filehandlerobjref.h"

void FileHandlerObjRefBase::increment(FileCacheHandler * const handler) const {
    handler->mReferenceCount++;
}

void FileHandlerObjRefBase::decrement(FileCacheHandler * const handler) const {
    handler->mReferenceCount--;
}
