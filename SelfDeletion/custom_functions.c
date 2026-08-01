#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <tchar.h>
#include <processsnapshot.h>
#include <tlhelp32.h>
#include <string.h>


void SelfDelete(void) {

	const wchar_t NewStream[] = L":BILLY";
	SIZE_T RenameSize = sizeof(FILE_RENAME_INFO) + sizeof(NewStream);
	PFILE_RENAME_INFO pFileRenameInfo = NULL;
	pFileRenameInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RenameSize);
	//FILE_RENAME_INFO FileRenameInfo = { 0 };

	WCHAR PathName[MAX_PATH * 2] = { 0 };
	FILE_DISPOSITION_INFO SetDelete = { 0 };
	ZeroMemory(PathName, sizeof(PathName));
	ZeroMemory(&SetDelete, sizeof(FILE_DISPOSITION_INFO));

	SetDelete.DeleteFile = TRUE;	//Set File for deletion

	/* set members for FILE_RENAME_INFO struct */
	pFileRenameInfo->FileNameLength = wcslen(NewStream) * sizeof(wchar_t); //sizeof(NewStream) -> this included the \0 , which is wrong
	RtlCopyMemory(pFileRenameInfo->FileName, NewStream, sizeof(NewStream));
	pFileRenameInfo->ReplaceIfExists = FALSE;
	pFileRenameInfo->RootDirectory = NULL;


	if (GetModuleFileNameW(NULL, PathName, MAX_PATH * 2) == 0) {
		return;
	}
	/* -------------------- start DELETE I : Rename the primary $DATA stream to :BILLY --------------------*/
	HANDLE hFile = CreateFileW(PathName, (DELETE | SYNCHRONIZE), FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("[x] first CreateFileW() failed, err -> %d\n", GetLastError());
		return;
	}

	/*
	THERE WAS ISSUE IN PARAMETERS HERE
		solved by correctly assiging pFileRenameInfo->FileNameLength correctly
	*/

	if (!SetFileInformationByHandle(hFile, FileRenameInfo, pFileRenameInfo, RenameSize)) {
		printf("[x] first SetFileInformationByHandle() failed, err-> %d\n", GetLastError());
		return;
	}

	CloseHandle(hFile);		// SAVE CHANGES
	/* -------------------- end DELETE I --------------------*/



	/* -------------------- start DELETE II: Mark file for deletion and Delete it --------------------*/
	WCHAR PathWithStream[MAX_PATH * 2] = { 0 };  // New buffer for path with stream
	wcscpy_s(PathWithStream, MAX_PATH * 2, PathName);
	wcscat_s(PathWithStream, MAX_PATH * 2, NewStream);

	hFile = CreateFileW(PathWithStream, (DELETE | SYNCHRONIZE), FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("[x] second CreateFileW() failed, err -> %d\n", GetLastError());
		return;
	}

	if (!SetFileInformationByHandle(hFile, FileDispositionInfo, &SetDelete, sizeof(SetDelete))) {
		printf("[x] second SetFileInformationByHandle() failed, err -> %d\n", GetLastError());
		return;
	}
	CloseHandle(hFile);

	if (DeleteFileW(PathName) == 0) { printf("[x] DeleteFileW() failed, err -> %d\n", GetLastError()); return; }
	//printf("[+] File should be deleted\n");

	/* -------------------- end DELETE II --------------------*/

	BOOL bHeapFree = HeapFree(GetProcessHeap(), 0, pFileRenameInfo);


	return;
}