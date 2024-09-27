/****************************************************************************

  Win32 File compatibility for DOS.
  [This version does support LFNs, if available.]

  Written by: Kenneth J. Davis
  Date:       August, 2000
  Contact:    jeremyd@computer.org


Copyright (c): Public Domain [United States Definition]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/


/*** Expects Pointers to be near (Tiny, Small, and Medium models ONLY) ***/

#include <dos.h>
#include <stdio.h>   /* PATH_MAX */
#include <stdlib.h>
#include <string.h>

#include "dosdisk.h"

#define searchAttr ( FILE_A_SUBDIR | FILE_A_HIDDEN | FILE_A_SYSTEM | FILE_A_RDONLY | FILE_A_ARCH )


struct FFDTA *FindFirstFile(const char *pathname, struct FFDTA *hnd) {
  char path[PATH_MAX];
  short cflag = 0;  /* used to indicate if findfirst is succesful or not */

  /* initialize structure (clear) */
  /* hnd->handle = 0;  hnd->ffdtaptr = NULL; */
  memset(hnd, 0, sizeof(*hnd));

  /* if pathname ends in \* convert to \*.* */
  strcpy(path, pathname);
  {
  int eos = strlen(path) - 1;
  if ((path[eos] == '*') && (path[eos - 1] == '\\')) strcat(path, ".*");
  }

  {
    unsigned short ffdta_seg, ffdta_off;
    unsigned short path_seg, path_off;
    unsigned short sattr = searchAttr;
    ffdta_seg = FP_SEG(hnd);
    ffdta_off = FP_OFF(hnd);
    path_seg = FP_SEG(path);
    path_off = FP_OFF(path);

  _asm {
    push ax
    push bx
    push cx
    push dx
    push es

    mov ah, 0x2F                   //; Get Current DTA
    int 0x21                       //; Execute interrupt, returned in ES:BX
    push bx                        //; Store its Offset, then Segment
    push es
    mov ah, 0x1A                   //; Set Current DTA to our buffer, DS:DX
    mov dx, ffdta_off
    push ds
    mov ds, ffdta_seg
    int 0x21                       //; Execute interrupt
    pop ds
    mov ax, 0x4E00                 //; Actual findfirst call
    mov cx, sattr
    mov dx, path_off               //; Load DS:DX with pointer to path for Findfirt
    push ds
    mov ds, path_seg
    int 0x21                       //; Execute interrupt
    pop ds
    jnc success                    //; If carry is not set then succesful
    mov [cflag], ax                //; Set flag with error.
success:
    mov ah, 0x1A              //; Set Current DTA back to original, DS:DX
    mov dx, ds                //; Store DS, must be preserved
    pop ds                    //; Popping ES into DS since thats where we need it.
    pop bx                    //; Now DS:BX points to original DTA
    int 0x21                  //; Execute interrupt to restore.
    mov ds, dx                //; Restore DS

    pop es
    pop dx
    pop cx
    pop bx
    pop ax
  }
  }

  if (cflag) return(NULL);

  return hnd;
}


int FindNextFile(struct FFDTA *hnd) {
  short cflag = 0;  /* used to indicate if dos findnext succesful or not */

  /* if bad handle given return */
  if (hnd == NULL) return 0;

  { /* Use DOS (0x4F) findnext, returning if error */
    unsigned short dta_seg = FP_SEG(hnd);
    unsigned short dta_off = FP_OFF(hnd);
    _asm {
      push ax
      push bx
      push cx
      push dx
      push es

      mov ah, 0x2F                  //; Get Current DTA
      int 0x21                      //; Execute interrupt, returned in ES:BX
      push bx                       //; Store its Offset, then Segment
      push es
      mov ax, 0x1A00                //; Set Current DTA to our buffer, DS:DX
      mov dx, dta_off
      push ds
      mov ds, dta_seg
      int 0x21                      //; Execute interrupt
      pop ds
      mov ax, 0x4F00                //; Actual findnext call
      int 0x21                      //; Execute interrupt
      JNC success                   //; If carry is not set then succesful
      mov [cflag], ax               //; Set flag with error.
success:
      mov ah, 0x1A                  //; Set Current DTA back to original, DS:DX
      MOV dx, ds                    //; Store DS, must be preserved
      pop ds                        //; Popping ES into DS since thats where we need it.
      pop bx                        //; Now DS:BX points to original DTA
      int 0x21                      //; Execute interrupt to restore.
      mov ds, dx                    //; Restore DS

      pop es
      pop dx
      pop cx
      pop bx
      pop ax
    }
  }

  if (cflag) return 0;

  return 1;
}


/* free resources to prevent memory leaks */
void FindClose(struct FFDTA *hnd) {
  /* 1st check if valid handle given */
  if (hnd == NULL) return;
}


/**
 Try LFN getVolumeInformation 1st
 if successful, assume valid drive/share (ie will return 1 unless error getting label)
 if failed (any error other than unsupported) return 0
 if a drive (ie a hard drive, ram drive, network mapped to letter) [has : as second letter]
 {
   try findfirst for volume label. (The LFN api does not seem to support LABEL attribute searches.)
     If getVolInfo unsupported but get findfirst succeed, assume valid (ie return 1)
   Try Get Serial#
 }
 else a network give \\server\share and LFN getVol unsupported, assume failed 0, as the
   original findfirst/next I haven't seen support UNC naming, clear serial and volume.
*** Currently trying to find a way to get a \\server\share 's serial & volume if LFN available ***
*/

/* returns zero on failure, if lpRootPathName is NULL or "" we use current
 * default drive. */
int GetVolumeInformation(const char *lpRootPathName, char *lpVolumeNameBuffer,
  size_t nVolumeNameSize, unsigned long *lpVolumeSerialNumber) {

  /* Using DOS interrupt to get serial number */
  struct media_info {
    short dummy;
    unsigned long serial;
    char volume[11];
    short ftype[8];
  } media;

  /* Stores the root path we use. */
  char pathname[PATH_MAX + 5];

  unsigned short cflag;

  /* validate root path to obtain info on, NULL or "" means use current */
  if ((lpRootPathName == NULL) || (*lpRootPathName == '\0')) {
    /* Assume if NULL user wants current drive, eg C:\ */
    _asm {
      push ax
      push bx

      mov ah, 0x19            //; Get Current Default Drive
      int 0x21                //; returned in AL, 0=A, 1=B,...
      lea bx, pathname        //; load pointer to our buffer
      add al, 'A'             //; Convert #returned to a letter
      mov [bx], al            //; Store drive letter
      inc bx                  //; point to next character
      mov byte ptr [bx], ':'  //; Store the colon
      inc bx
      mov byte ptr [bx], '\'  //; this gets converted correctly as a single bkslsh
      inc bx
      mov byte ptr [bx], 0    //; Most importantly the '\0' terminator

      pop bx
      pop ax
    }
  } else {
    strcpy(pathname, lpRootPathName);
  }

  /* if a drive, test if valid, get volume, and possibly serial # */

  /* assume these calls will succeed, change on an error */
  cflag = 0;

  /* get path ending in \*.*, */
  if (pathname[strlen(pathname)-1] != '\\') {
    strcat(pathname, "\\*.*");
  } else {
    strcat(pathname, "*.*");
  }

  /* Search for volume using old findfirst, as LFN version (NT5 DOS box) does
   * not recognize FILE_A_VOL = 0x0008 as label attribute.
   */
  {
    struct FFDTA finfo;
    unsigned short attr_seg = FP_SEG(finfo.attrib);
    unsigned short attr_off = FP_OFF(finfo.attrib);
    unsigned short finfo_seg = FP_SEG(&finfo);
    unsigned short finfo_off = FP_OFF(&finfo);
    unsigned short pathname_seg = FP_SEG(pathname);
    unsigned short pathname_off = FP_OFF(pathname);

    _asm {
      push ax
      push bx
      push cx
      push dx
      push es

      mov ah, 0x2F              //; Get Current DTA
      int 0x21                  //; Execute interrupt, returned in ES:BX
      push bx                   //; Store its Offset, then Segment
      push es
      mov ah, 0x1A              //; Set Current DTA to our buffer, DS:DX
      mov dx, finfo_off         //; Load our buffer for new DTA.
      push ds
      mov ds, finfo_seg
      int 0x21                  //; Execute interrupt
      pop ds
      mov ax, 0x4E00            //; Actual findfirst call
      mov cx, FILE_A_VOLID
      mov dx, pathname_off      //; Load DS:DX with pointer to modified RootPath for Findfirt
      push ds
      mov ds, pathname_seg
      int 0x21                  //; Execute interrupt, Carry set on error, unset on success
      pop ds
      jnc success               //; If carry is not set then succesful
      mov [cflag], ax           //; Set flag with error.
      jmp cleanup               //; Restore DTA
    success:                    //; True volume entry only has volume attribute set [MS' LFNspec]
      mov bx, attr_off
      push es
      mov es, attr_seg
      mov al, [es:bx]              //; Looking for a BYTE that is FILE_ATTRIBUTE_LABEL only
      pop es
      and al, 0xDF              //; Ignore Archive bit
      cmp al, FILE_A_VOLID
      je cleanup                //; They match, so should be true volume entry.
      mov ax, 0x4F00            //; Otherwise keep looking (findnext)
      int 0x21                  //; Execute interrupt
      jnc success               //; If carry is not set then succesful
      mov [cflag], ax           //; Set flag with error.
    cleanup:
      mov ah, 0x1A              //; Set Current DTA back to original, DS:DX
      mov dx, ds                //; Store DS, must be preserved
      pop ds                    //; Popping ES into DS since thats where we need it.
      pop bx                    //; Now DS:BX points to original DTA
      int 0x21                  //; Execute interrupt to restore.
      mov ds, dx                //; Restore DS

      pop es
      pop dx
      pop cx
      pop bx
      pop ax
    }

    /* copy over volume label, if buffer given */
    if (lpVolumeNameBuffer != NULL) {
      if (cflag != 0) {  /* error or no label */
        lpVolumeNameBuffer[0] = '\0';
      } else {                      /* copy up to buffer's size of label */
        strncpy(lpVolumeNameBuffer, finfo.name, nVolumeNameSize);
        lpVolumeNameBuffer[nVolumeNameSize-1] = '\0';
        /* slide characters over if longer than 8 to remove . */
        if (lpVolumeNameBuffer[8] == '.') {
          lpVolumeNameBuffer[8] = lpVolumeNameBuffer[9];
          lpVolumeNameBuffer[9] = lpVolumeNameBuffer[10];
          lpVolumeNameBuffer[10] = lpVolumeNameBuffer[11];
          lpVolumeNameBuffer[11] = '\0';
        }
      }
    }
  }

  /* Test for no label found, which is not an error,
     Note: added the check for 0x02 as FreeDOS returns this instead
     at least for disks with LFN entries and no volume label.
  */
  if ((cflag == 0x12) || /* No more files or   */
      (cflag == 0x02)) {  /* File not found     */
    cflag = 0;       /* so assume valid drive  */
  }

  /* Get Serial Number, only supports drives mapped to letters */
  media.serial = 0;         /* set to 0, stays 0 on an error */
  {
    unsigned short media_off = FP_OFF(&media);
    unsigned short media_seg = FP_SEG(&media);
    unsigned char drv = (pathname[0] & 0xDF) - 'A' + 1;
    _asm {
      push ax
      push bx
      push cx
      push dx

      xor bh, bh
      mov bl, drv             //; Clear BH, drive in BL
      mov cx, 0x0866          //; CH=disk drive, CL=Get Serial #
      mov ax, 0x440D          //; Generic IOCTL
      mov dx, media_off       //; DS:DX pointer to media structure
      push ds
      mov ds, media_seg
      int 0x21
      pop ds

      pop dx
      pop cx
      pop bx
      pop ax
    }
  }

  if (lpVolumeSerialNumber != NULL) *lpVolumeSerialNumber = media.serial;

  /* If there was an error getting the validating drive return failure) */
  if (cflag) {  /* cflag is nonzero on any errors )*/
    return 0;   /* zero means error! */
  } else {
    return 1;   /* Success (drive exists we think anyway) */
  }
}
