/* NOTE: this file is included directly in tree.c */

/* Based on streams.cpp by Felix Kasza
 * as best I can tell last modified May 1998
 * original code may be found at http://win32.mvps.org/index.html
 * License statement indicates it is public domain
 */

//
// Define the file information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.
//

typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileCopyOnWriteInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileOleClassIdInformation,
	FileOleStateBitsInformation,
	FileNetworkOpenInformation,
	FileObjectIdInformation,
	FileOleAllInformation,
	FileOleDirectoryInformation,
	FileContentIndexInformation,
	FileInheritContentIndexInformation,
	FileOleInformation,
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

//
// Define the various structures which are returned on query operations
//

typedef struct _FILE_BASIC_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG NumberOfLinks;
	BOOLEAN DeletePending;
	BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION {
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
	ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
	BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
	LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;


typedef struct _FILE_FULL_EA_INFORMATION {
	ULONG NextEntryOffset;
	UCHAR Flags;
	UCHAR EaNameLength;
	USHORT EaValueLength;
	CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;


struct FILE_STREAM_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG StreamNameLength;
	LARGE_INTEGER StreamSize;
	LARGE_INTEGER StreamAllocationSize;
	WCHAR StreamName[1];
};


//
// Define the base asynchronous I/O argument types
//

typedef LONG NTSTATUS;

typedef struct _IO_STATUS_BLOCK {
	NTSTATUS Status;
	ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


DWORD __stdcall NtQueryInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

typedef DWORD (__stdcall *NQIF)(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

static NQIF pNtQueryInformationFile = NULL;


#define DefaultStreamNameLengthBytes (7*2)
static char DefaultStreamName[DefaultStreamNameLengthBytes];

/* Returns a pointer to a static buffer containing the
 * information about the streams available for a given
 * path.  Path should be fully qualified.
 * On error or if the needed function is not available (eg
 * on Win9x systems) will return NULL.
 * As the returned pointer refers to a static buffer, its
 * contents will be overwritten on subsequent calls, but
 * the user need never worry about allocating/deallocating
 * memory necessary for it - presently we support at most
 * 32KB of stream data associated with a file.
 * Path is assumed to be in ASCII(CP_ACP) form.
 */
FILE_STREAM_INFORMATION *getFileStreamInfo(const char *path)
{
  static byte fsibuf[32768];
  FILE_STREAM_INFORMATION *fsi = (FILE_STREAM_INFORMATION *)fsibuf;
  HANDLE h;
  IO_STATUS_BLOCK iosb;

  /* exit early if function not available */
  if (pNtQueryInformationFile == NULL) return NULL;

#if 0
  /* convert path to Unicode for call, temp use fsibuf */
  MultiByteToWideChar(CP_ACP, 0, path, -1, fsibuf, sizeof(fsibuf));
#endif

  /* note that while we do have to open the file, not even read access is needed */
  h = CreateFileA(path, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (h == INVALID_HANDLE_VALUE) return NULL;

  /* now for the undocumented call that gets us the needed information */
  if (pNtQueryInformationFile(h, &iosb, &fsibuf, sizeof(fsibuf), FileStreamInformation) != 0)
  {
    fsi = NULL;
  }
  CloseHandle(h);

  /* return pointer to our static buffer that was filled in */
  return fsi;
}


void initStreamSupport(void)
{
  HINSTANCE hNtdll;  /* Handle to module with NtQueryInformationFile function */

  /* init name of default stream, could do this statically in the future */
  MultiByteToWideChar(CP_ACP, 0, "::$DATA", -1, (unsigned short*)DefaultStreamName, 7);

  /* get reference to module our function is in */
  hNtdll = LoadLibrary("ntdll.dll");
  if (hNtdll < (HINSTANCE)33) return;

  /* actually aquire a pointer to the function */
  pNtQueryInformationFile = (NQIF)GetProcAddress(hNtdll, "NtQueryInformationFile");

  /* we assume if NTDLL.DLL is available, it is always loaded so we don't keep our reference*/
  FreeLibrary(hNtdll);
}
