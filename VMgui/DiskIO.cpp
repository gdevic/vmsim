// DiskIO.cpp : implementation file
//

#include "stdafx.h"
#include "VMgui.h"

#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"
#include "InstDrv.h"
#include "Functions.h"                  // Include all the function decl

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include "Simulator.h"
#include "ThreadMessages.h"
#include "ThreadMgr.h"
#include "Progress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CSimulator::DiskDisableError(PDISKCONFIG pDisk, const char *pMessage)
{
    CString s;

    pDisk->Flags &= ~DSK_ENABLED;

    if( pDisk->hFile   )    close(pDisk->hFile);
    if( pDisk->fpCache )    fclose(pDisk->fpCache);
    if(pDisk->pCacheIndex)  delete pDisk->pCacheIndex;

    pDisk->hFile   = 0;
    pDisk->fpCache = NULL;
    pDisk->pCacheIndex = NULL;

    s.Format("Error: %s\r", pMessage);
    s += _T("A virtual disk has been disabled.");
    AfxMessageBox(s, MB_OK);
}


// Opens a single virtual disk

void CSimulator::DiskOpen(PDISKCONFIG pDisk)
{
    CString msg;

    // Return if a disk is disabled
    if(!(pDisk->Flags & DSK_ENABLED))
        return;

    // If a disk is virtual, open its file.
    // If a disk is physical, lock it
    if(pDisk->Flags & DSK_VIRTUAL)
    {
        // Virtual disk
        pDisk->hFile = open( pDisk->PathToFile, _O_CREAT | _O_RDWR | _O_BINARY );
        if(pDisk->hFile <= 0)
        {
            // Error opening virtual disk.. disable it.
            msg.Format("Unable to open disk file %s", pDisk->PathToFile);
            DiskDisableError(pDisk, (LPCTSTR) msg);

            return;
        }
    }
    else
    {
        // Physical disk
        ASSERT(0);
    }

    // If a disk is undoable, initialize that capability
    if(pDisk->Flags & DSK_WRITECACHE)
    {
        pDisk->fpCache = tmpfile();

        pDisk->pCacheIndex = new DWORD [pDisk->max_sectors];
        memset(pDisk->pCacheIndex, 0, pDisk->max_sectors * sizeof(DWORD));

        if(pDisk->fpCache==NULL || pDisk->pCacheIndex==NULL)
        {
            // Error creating temp file or allocating memory

            msg.Format("Unable to initialize undoable file for virtual disk %s\r\
                        Would you like to make this disk not-undoable?", pDisk->PathToFile);
            if( AfxMessageBox(msg, MB_YESNO | MB_ICONSTOP)==IDYES )
            {
                // Make this disk not-undoable
                pDisk->Flags &= ~DSK_WRITECACHE;
                if(pDisk->pCacheIndex) delete pDisk->pCacheIndex;

                msg.Format("Disk %s has been made not-undoable", pDisk->PathToFile);
                AfxMessageBox(msg, MB_OK);
            }
            else
            {
                // Disable this disk
                DiskDisableError(pDisk, "Disk has been disabled");
            }
        }
    }
}

// Opens all virtual disks for access

void CSimulator::DiskOpenAll()
{
    PDISKCONFIG pDisk;

    for( int n=0; n<MAX_DISKS; n++)
    {
        pDisk = &pTheApp->m_VMConfig.Disk[n];
        DiskOpen(pDisk);
    }
}

// Flushes and closes all virtual disks after virtual machine has stopped

void CSimulator::DiskCloseAll()
{
    PDISKCONFIG pDisk;
    CString msgbuf;
    DWORD sector;
    BYTE buffer[512];
    int num_dirty, count_clean, hCache;
    int pos_source, pos_target, bytes;

    for( int n=0; n<MAX_DISKS; n++ )
    {
        pDisk = &pTheApp->m_VMConfig.Disk[n];

        // Close only valid enabled disks
        if(pDisk->Flags & DSK_ENABLED)
        {
            // If a disk was undoable, we also got a write cache file that we
            // need to ask if we should write it back
            if(pDisk->Flags & DSK_WRITECACHE)
            {
                // This is also for the smoothness of the progress bar indicator:
                // find how many sectors we will write in order to set the bounds

                for( num_dirty = sector = 0; sector < pDisk->max_sectors; sector++ )
                {
                    if( pDisk->pCacheIndex[sector] != 0 )
                        num_dirty++;
                }

                if( num_dirty > 0 )
                {
                    msgbuf = TEXT("Virtual disk file has been modified.  Commit changes?");
                    if( AfxMessageBox(msgbuf, MB_YESNO | MB_ICONQUESTION)==IDYES )
                    {
                        // OK, we are clear to commit changes
                        ASSERT(pDisk->pCacheIndex);
                        ASSERT(pDisk->fpCache);

                        hCache = fileno(pDisk->fpCache);

                        msgbuf = TEXT("Commiting changes to disk ") + pDisk->PathToFile;
                        CProgress *pProgress = new CProgress();
                        pProgress->Create();
                        pProgress->SetRange(0, num_dirty);
                        pProgress->SetText((LPCTSTR) msgbuf );

                        // pCacheIndex is an array of DWORDS that are offsets into the
                        // hCache file on where the sector is cached.

                        for( count_clean = sector = 0; sector < pDisk->max_sectors; sector++, count_clean++ )
                        {
                            if( pDisk->pCacheIndex[sector] != 0)
                            {
                                // Set the progress bar index
                                pProgress->SetPos(count_clean);

                                // Ok, this sector has been cached; write it back
                                // Set the position on the source and target file
                                pos_source = lseek(hCache, pDisk->pCacheIndex[sector] - 1, SEEK_SET);
                                pos_target = lseek(pDisk->hFile, sector * 512, SEEK_SET);

                                if((pos_source != (int) pDisk->pCacheIndex[sector] - 1)
                                    || (pos_target != (int) sector * 512))
                                {
                                    ASSERT(0);
                                }

                                // Read in the cached sector and write it into the target file
                                bytes = read(hCache, buffer, 512);
                                if(bytes != 512)
                                {
                                    ASSERT(0);
                                }

                                bytes = write(pDisk->hFile, buffer, 512);
                                if(bytes != 512)
                                {
                                    ASSERT(0);
                                }
                            }
                        }

                        delete pProgress;
                    }
                }
                // Clear the cache index and close/delete the cache file
                memset(pDisk->pCacheIndex, 0, pDisk->max_sectors * sizeof(DWORD));
                fclose(pDisk->fpCache);
                pDisk->fpCache = NULL;
            }

            if(pDisk->Flags & DSK_VIRTUAL)
            {
                // Virtual disk
                ASSERT(pDisk->hFile > 0);
                close(pDisk->hFile);
                pDisk->hFile = 0;
            }
            else
            {
                // Physical disk
                ASSERT(0);
            }
        }
    }
}


// Helper function - reads specified number of consecutive sectors from the virtual disk
// or from the physical device.  Parameter num_sectors may be omitted to read only 1 sector.

BOOL CSimulator::DiskReadSectors(PDISKCONFIG pDisk, int offset, BYTE *pBuffer, int num_sectors)
{
    if(pDisk->Flags & DSK_VIRTUAL)
    {
        ASSERT(pDisk->hFile > 0);
        if(lseek(pDisk->hFile, offset, SEEK_SET) == offset)
        {
            if( read(pDisk->hFile, pBuffer, num_sectors * 512) == num_sectors * 512)
            {
            }
            else
            {
                // We could not read specified sectors
                ASSERT(0);
            }
        }
        else
        {
            // Error seeking the disk
            DiskDisableError(pDisk, "There has been an error seeking a virtual disk.");

            return FALSE;
        }
    }
    else
    {
        // Read sectors from the physical device
        ASSERT(0);
    }

    return TRUE;
}


// pVMReq contains the description of sectors that needs to be read
// or written in this fields:
//
//  pVMReq->head.drive   - the logical drive number: 0,1 floppy, 2,3,4,5 hard disks
//  pVMReq->head.offset  - logical offset of the first sector in bytes
//  pVMReq->head.sectors - number of sectors to read
//
void CSimulator::DiskRead( VMREQUEST *pVmReq )
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb, len, readden;
    PDISKCONFIG pDisk;
    CString msgbuf;
    BYTE *pSectorBuffer;
    int read_cache_pos, sector_index, hCache;

    ASSERT(pVmReq->head.sectors);
    ASSERT(pVmReq->head.drive < 6);
    pDisk = &pTheApp->m_VMConfig.Disk[pVmReq->head.drive];
    len = pVmReq->head.sectors * 512;

    if( pDisk->Flags & DSK_ENABLED )
    {
        // Make sure the buffer that we return is large enough for the
        // request number of sectors to fit in
        if( m_SectorBufferLen < len )
        {
            m_pSectorBuffer = (BYTE *) realloc(m_pSectorBuffer, len);
            m_SectorBufferLen = len;
            ASSERT(m_pSectorBuffer);
        }

        if(pDisk->Flags & DSK_WRITECACHE)
        {
            // See if the requested sector is already in the write cache
            ASSERT(pDisk->fpCache);
            ASSERT(pDisk->max_sectors);
            ASSERT(pDisk->pCacheIndex);

            // Each sector is possibly cached

            readden = 0;
            pSectorBuffer = m_pSectorBuffer;
            hCache = fileno(pDisk->fpCache);
            sector_index = pVmReq->head.offset / 512;
            ASSERT(sector_index <= (int)pDisk->max_sectors);

            do
            {
                // Is the sector in the write cache?
                if( pDisk->pCacheIndex[ sector_index ] & 1)
                {
                    read_cache_pos = pDisk->pCacheIndex[ sector_index ] & ~0x1FF;
                    if( read_cache_pos == lseek(hCache, read_cache_pos, SEEK_SET))
                    {
                        if( read(hCache, pSectorBuffer, 512) != 512)
                        {
                            // Error reading from the cache file
                            ASSERT(0);
                        }
                    }
                    else
                    {
                        // Error seeking into the cache file cached sector
                        ASSERT(0);
                    }
                }
                else    // Sector is not cached... Retrieve it using standard procedure
                {
                    DiskReadSectors(pDisk, sector_index * 512, pSectorBuffer);
                }

                // Get to the next sector to read in
                pSectorBuffer += 512;
                sector_index  += 1;
                readden       += 1;

            } while( readden < pVmReq->head.sectors );
        }
        else
        {
            // Not cacheable.  Read from the virtual disk or device
            DiskReadSectors(pDisk, pVmReq->head.offset, m_pSectorBuffer, pVmReq->head.sectors);
        }

        // Everything went ok, send the sector down to the driver

        status = DeviceIoControl( sys_handle, VMSIM_CompleteVMRequest,
            pVmReq, sizeof(VMREQUEST),
            m_pSectorBuffer, len,
            &nb, &overlap );
        status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
        if(!status)
            status = GetLastError();
    }
    else
    {
        // Accessing disabled drive from the device driver
        ASSERT(0);
    }
}


// pVMReq contains the description of sectors that needs to be read
// or written in this fields:
//
//  pVMReq->head.drive   - the logical drive number: 0,1 floppy, 2,3,4,5 hard disks
//  pVMReq->head.offset  - logical offset of the first sector in bytes
//  pVMReq->head.sectors - number of sectors to write
//
void CSimulator::DiskWrite( VMREQUEST *pVmReq )
{
    OVERLAPPED overlap = { 0 };
    DWORD status, nb, len, written;
    PDISKCONFIG pDisk;
    CString msgbuf;
    int pos, hCache, write_cache_pos, sector_index;
    BYTE *pSectorBuffer;

    ASSERT(pVmReq->head.sectors);
    ASSERT(pVmReq->head.drive < 6);
    pDisk = &pTheApp->m_VMConfig.Disk[pVmReq->head.drive];
    len = pVmReq->head.sectors * 512;

    if( pDisk->Flags & DSK_ENABLED )
    {
        // Make sure the buffer that we return is large enough for the
        // request number of sectors to fit in
        if( m_SectorBufferLen < len )
        {
            m_pSectorBuffer = (BYTE *) realloc(m_pSectorBuffer, len);
            m_SectorBufferLen = len;
            ASSERT(m_pSectorBuffer);
        }

        // Send the empty buffer down to the driver to be filled up
        status = DeviceIoControl( sys_handle, VMSIM_CompleteVMRequest,
            pVmReq, sizeof(VMREQUEST),
            m_pSectorBuffer, len,
            &nb, &overlap );
        status = GetOverlappedResult(sys_handle, &overlap, &nb, TRUE);      // wait
        if(!status)
            status = GetLastError();

        if(pDisk->Flags & DSK_WRITECACHE)
        {
            ASSERT(pDisk->fpCache);
            ASSERT(pDisk->max_sectors);
            ASSERT(pDisk->pCacheIndex);

            // Write sectors to the cache file instead.  Cache file grows as
            // new sectors are added
            written = 0;
            pSectorBuffer = m_pSectorBuffer;
            hCache = fileno(pDisk->fpCache);
            sector_index = pVmReq->head.offset / 512;
            ASSERT(sector_index <= (int)pDisk->max_sectors);

            do
            {
                // Is the sector already in the write cache?
                if( pDisk->pCacheIndex[ sector_index ] & 1)
                {
                    write_cache_pos = pDisk->pCacheIndex[ sector_index ] & ~0x1FF;
                    if( write_cache_pos != lseek(hCache, write_cache_pos, SEEK_SET))
                    {
                        // Error seeking into the cache file already cached sector
                        ASSERT(0);
                    }
                }
                else    // We need to create a new entry in the cache
                {
                    // Seek to the end of the cache file and so get that offset as well
                    write_cache_pos = lseek(hCache, 0, SEEK_END);
                    ASSERT((write_cache_pos & 0x1FF)==0);

                    // Set a new cache index
                    pDisk->pCacheIndex[ sector_index ] = write_cache_pos | 1;
                }

                // Write the sector into the cache file
                if( write(hCache, pSectorBuffer, 512) != 512)
                {
                    // Error writing into the sector buffer
                    ASSERT(0);
                }

                // Get to the next sector to write
                pSectorBuffer += 512;
                sector_index  += 1;
                written       += 1;

            } while( written < pVmReq->head.sectors );
        }
        else
            if(pDisk->Flags & DSK_VIRTUAL)
        {
            ASSERT(pDisk->hFile > 0);

                pos = lseek(pDisk->hFile, pVmReq->head.offset, SEEK_SET);
            if(pos==(long)pVmReq->head.offset)
            {
                if( write(pDisk->hFile, m_pSectorBuffer, len) != (int)len)
                {
                    // We could not write specified sectors
                    ASSERT(0);
                }
            }
            else
            {
                // Error seeking the disk
                DiskDisableError(pDisk, "There has been an error seeking a virtual disk.");
            }
        }
        else
        {
            // OK, but accessing physical disk
            ASSERT(0);
        }
    }
    else
    {
        // Accessing disabled drive from the device driver
        ASSERT(0);
    }
}

