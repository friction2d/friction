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

#include "memorychecker.h"


#if defined(Q_OS_WIN)
    #include "windowsincludes.h"
#elif defined(Q_OS_UNIX)
    #if defined(Q_OS_LINUX)
        #include "gperftools/tcmalloc.h"
        #include "../gperftools/include/gperftools/malloc_extension.h"
        #include <sys/sysinfo.h>
        #include <unistd.h>
    #elif defined(Q_OS_MACOS)
        #include <mach/mach_host.h>
        #include <mach/mach_init.h>
        #include <mach/host_info.h>
        #include <mach/task.h>
        #include <mach/vm_statistics.h>
        #include <sys/time.h>
        #include <sys/resource.h>
    #endif
#endif

#include <QDebug>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include "exceptions.h"
#include "hardwareinfo.h"
MemoryChecker *MemoryChecker::mInstance;

MemoryChecker::MemoryChecker(QObject * const parent) : QObject(parent) {
    mInstance = this;

    const intKB totRam = HardwareInfo::sRamKB();
    mLowFreeKB = totRam*20/100;
    mVeryLowFreeKB = totRam*15/100;
    mCriticalFreeKB = totRam*10/100;
}

char MemoryChecker::sLine[256];

void MemoryChecker::sGetFreeKB(intKB& procFreeKB,
                               intKB& sysFreeKB,
                               intKB& usedKB)
{
    const auto usageCap = eSettings::sInstance->fRamMBCap;

    longB enveUsedB(0);
    qint64 freeInternal = 0;
    intKB freeExternal(0);

#if defined(Q_OS_WIN)
    const auto processID = GetCurrentProcessId();
    const auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                      FALSE, processID);

    if(hProcess != NULL) {
        PROCESS_MEMORY_COUNTERS pmc;
        if(GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            enveUsedB = longB(pmc.WorkingSetSize);
        }
    }

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    const longB availPhysB(statex.ullAvailPhys);
    freeExternal = intKB(availPhysB);
#elif defined(Q_OS_UNIX)
    size_t bytes_in_use_by_app = 0;
#if defined(Q_OS_LINUX)
    size_t physical_memory_used;
    size_t virtual_memory_used;
    MallocExtension::instance()->eMemoryStats(&virtual_memory_used,
                                              &physical_memory_used,
                                              &bytes_in_use_by_app);
#elif defined(Q_OS_MACOS)
    task_vm_info_data_t vm_info;
    mach_msg_type_number_t vm_info_count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(),
                  TASK_VM_INFO,
                  (task_info_t)&vm_info,
                  &vm_info_count) == KERN_SUCCESS) {
        bytes_in_use_by_app = vm_info.phys_footprint;
    }
#endif

    enveUsedB = longB(static_cast<qint64>(bytes_in_use_by_app));

#if defined(Q_OS_LINUX)
        freeInternal = physical_memory_used - bytes_in_use_by_app;
        int found = 0;
        FILE * const meminfo = fopen("/proc/meminfo", "r");
        if (!meminfo) { RuntimeThrow("Failed to open /proc/meminfo"); }
        while(fgets(sLine, sizeof(sLine), meminfo)) {
            int ramPartKB;
            if (sscanf(sLine, "MemFree: %d kB", &ramPartKB) == 1) {
            } else if(sscanf(sLine, "Cached: %d kB", &ramPartKB) == 1) {
            } else if(sscanf(sLine, "Buffers: %d kB", &ramPartKB) == 1) {
            } else { continue; }
            freeExternal.fValue += ramPartKB;
            if(++found == 3) break;
        }
        fclose(meminfo);
        if(found != 3) RuntimeThrow("Entries missing from /proc/meminfo");
#elif defined(Q_OS_MACOS)
        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        vm_statistics_data_t vmstat;
        const auto ret = host_statistics(mach_host_self(), HOST_VM_INFO,
                                         (host_info_t)&vmstat, &count);
        if (ret != KERN_SUCCESS) { RuntimeThrow("Could not retrieve memory usage"); }
        const int pageSize = getpagesize() / 1024;
        freeExternal.fValue += vmstat.inactive_count * pageSize;
        freeExternal.fValue += vmstat.free_count * pageSize;
#endif
#endif

    const intKB enveUsedKB(enveUsedB);
    if (usageCap.fValue > 0) {
        procFreeKB = intKB(usageCap) - enveUsedKB;
    } else {
        procFreeKB = HardwareInfo::sRamKB() - enveUsedKB;
    }

    sysFreeKB = intKB(longB(freeInternal)) + freeExternal;
    usedKB = enveUsedKB;
    //qDebug() << "Free" << intMB(sysFreeKB).fValue << "Used" << intMB(enveUsedKB).fValue;

#if defined(Q_OS_LINUX)
    const qint64 releaseBytes = 500L*1024L*1024L;
    if (freeInternal > releaseBytes) {
        MallocExtension::instance()->ReleaseToSystem(releaseBytes);
    }
#endif
}

void MemoryChecker::checkMemory() {
    intKB procFreeKB;
    intKB sysFreeKB;
    intKB usedKB;
    sGetFreeKB(procFreeKB, sysFreeKB, usedKB);

    if(sysFreeKB < mLowFreeKB) {
        const intKB toFree = mLowFreeKB - sysFreeKB;
        if(sysFreeKB < mCriticalFreeKB) {
            emit handleMemoryState(CRITICAL_MEMORY_STATE, longB(toFree));
            mLastMemoryState = CRITICAL_MEMORY_STATE;
        } else if(sysFreeKB < mVeryLowFreeKB) {
            emit handleMemoryState(VERY_LOW_MEMORY_STATE, longB(toFree));
            mLastMemoryState = VERY_LOW_MEMORY_STATE;
        } else {
            emit handleMemoryState(LOW_MEMORY_STATE, longB(toFree));
            mLastMemoryState = LOW_MEMORY_STATE;
        }
    } else if(procFreeKB.fValue < 0) {
        emit handleMemoryState(LOW_MEMORY_STATE, longB(-procFreeKB));
        mLastMemoryState = LOW_MEMORY_STATE;
    } else/* if(mLastMemoryState != NORMAL_MEMORY_STATE)*/ {
        emit handleMemoryState(NORMAL_MEMORY_STATE, longB(0));
        mLastMemoryState = NORMAL_MEMORY_STATE;
    }

    emit memoryCheckedKB(sysFreeKB, HardwareInfo::sRamKB(), usedKB);
}
