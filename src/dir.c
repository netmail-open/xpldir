#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <xplmem.h>
#include <xpldir.h>

#if defined(SOLARIS) || defined(LINUX) || defined(S390RH) || defined(MACOSX)

int XplMakeDir(const char *path)
{
	return mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
}

XplDir *XplOpenDir(const char *dirname)
{
	XplDir			*NewDir	= NULL;

	if (dirname) {
		NewDir = (XplDir*)malloc(sizeof(XplDir));

		if (NewDir)
		{
			strcpy(NewDir->Path, dirname);
			NewDir->d_attr	= 0;
			NewDir->d_name	= NULL;
			NewDir->direntp= NULL;
			NewDir->dirp	= opendir(dirname);
			if (NewDir->dirp==NULL) {
				free(NewDir);
				return(NULL);
			}
		}
	}
	return(NewDir);
}

XplDir *XplReadDir(XplDir *dirp)
{
	if (dirp && dirp->dirp)
	{
		dirp->direntp = readdir(dirp->dirp);
		if (dirp->direntp)
		{
			struct stat		Stats;
			unsigned char	Path[XPL_MAX_PATH+1];

			strprintf(Path, sizeof(Path), NULL, "%s/%s", dirp->Path, dirp->direntp->d_name);
			memset(&Stats, 0, sizeof(Stats));
			stat(Path, &Stats);
			dirp->d_attr		= Stats.st_mode;
			dirp->d_name		= dirp->direntp->d_name;
			dirp->d_cdatetime	= Stats.st_mtime;
			dirp->d_size		= Stats.st_size;

			return(dirp);
		}
	}
	return(NULL);
}

int XplIsSubDir(XplDir *dirp)
{
	if( dirp )
	{
		return S_ISDIR(dirp->d_attr);
	}
	return 0;
}

int XplCloseDir(XplDir *dirp)
{
	int retval = -1;

	if (dirp) {
		if (dirp->dirp) {
			retval = closedir(dirp->dirp);
		}

		free(dirp);
		dirp = NULL;
	}
	return(retval);
}

XplDirMatch * XplOpenDirMatch(const char *pattern)
{
	int count;
	char *end;
	char *match;
	DIR *dp;
	struct stat sb;
	struct dirent *de;
	XplDirMatch *entry;
	XplDirMatch *parent;
	char path[XPL_MAX_PATH + 1];

	do {
		dp = NULL;

		entry = NULL;
		parent = (XplDirMatch *)calloc(1, sizeof(XplDirMatch));
		if (parent) {
			if (pattern && ((end = strrchr(pattern, '/')) != NULL)) {
				end++;

				count = end - pattern;
				memcpy(path, pattern, count);

				match = (char *)pattern + count;
			} else {
				count = 2;
				memcpy(path, "./", 2);

				match = (char *)pattern;
			}

			end = path + count;
			*end = '\0';
		} else {
			break;
		}

		if (!(parent->d_name = strdup(path)) || !(dp = opendir(path))) {
			break;
		}

		while ((de = readdir(dp)) != NULL) {
			if ((memcmp(de->d_name, ".", 2)  != 0)
					&& (memcmp(de->d_name, "..", 3) != 0)
					&& (strpat(de->d_name, match) == 0)) {
				if (entry == NULL) {
					entry = (XplDirMatch *)calloc(1, sizeof(XplDirMatch));
				}

				if (entry != NULL) {
					strcpy(end, de->d_name);
					if (stat(path, &sb) == 0) {
						entry->d_name = strdup(de->d_name);
					} else {
						continue;
					}
				} else {
					break;
				}

				if (entry->d_name) {
					entry->d_size = sb.st_size;
					entry->d_cdatetime = sb.st_mtime;
					entry->d_attr = sb.st_mode;

					entry->base = parent;
					entry->next = parent->next;
					parent->next = entry;

					entry = NULL;
				} else {
					break;
				}
			}
		}

		closedir(dp);

		return(parent);
	} while (FALSE);

	if (dp) {
		closedir(dp);
	}

	if (entry) {

		if (entry->d_name) {
			free(entry->d_name);
		}

		free(entry);
	}

	while (parent) {
		entry = parent->next;

		if (parent->d_name) {
			free(parent->d_name);
		}

		free(parent);

		parent = entry;
	}

	return(NULL);
}

#elif defined(WIN32)


EXPORT XplDir * XplOpenDir(const char *dirname)
{
	XplDir			*NewDir	= NULL;
	struct stat		sb;

	if (dirname) {
		memset(&sb, 0, sizeof(sb));
		if (stat(dirname, &sb)) {
			/* The directory does not exist or can't be accessed */
			return(NULL);
		}
	}

	if (dirname) {
		NewDir = (XplDir*)malloc(sizeof(XplDir));
		if (NewDir) {
			if (dirname && (strlen(dirname)<(XPL_MAX_PATH-4))) {
				sprintf(NewDir->Path, "%s/*.*", dirname);
			} else {
				free(NewDir);
				return(NULL);
			}
			NewDir->d_attr	= 0;
			NewDir->d_name	= NULL;
			NewDir->dirp	= -1;
		}
	}
	return(NewDir);
}

EXPORT XplDir * XplReadDir(XplDir *dirp)
{
	if (dirp) {
		if (dirp->dirp!=-1) {
			if (_findnext(dirp->dirp, &(dirp->FindData))!=-1) {
				dirp->d_attr	   = dirp->FindData.attrib;
				dirp->d_name	   = dirp->FindData.name;
				dirp->d_cdatetime = dirp->FindData.time_write;
				dirp->d_size	   = dirp->FindData.size;
				return(dirp);
			}
		} else {
			dirp->dirp	= _findfirst(dirp->Path, &(dirp->FindData));
			if (dirp->dirp!=-1) {
				dirp->d_attr	   = dirp->FindData.attrib;
				dirp->d_name	   = dirp->FindData.name;
				dirp->d_cdatetime = dirp->FindData.time_write;
				dirp->d_size	   = dirp->FindData.size;
				return(dirp);
			}
		}
	}

	return(NULL);
}

EXPORT int XplIsSubDir(XplDir *dirp)
{
	if( dirp )
	{
		return (dirp->d_attr & _A_SUBDIR);
	}
	return 0;
}

EXPORT int XplCloseDir(XplDir *dirp)
{
	int retval = -1;

	if (dirp) {
		if (dirp->dirp!=-1) {
			retval = _findclose(dirp->dirp);
		}
		free(dirp);
		dirp = NULL;
	}

	return(retval);
}

/*
	These directory pattern matching interfaces were based on
	recommendations by the Microsoft(r) UNIX code migration guide.
*/
EXPORT XplDirMatch * XplOpenDirMatch(const char *pattern)
{
	int count;
	char *end;
	XplBool finished;
	HANDLE hList;
	WIN32_FIND_DATA hData;
	XplDirMatch *entry;
	XplDirMatch *parent;
	char path[XPL_MAX_PATH + 1];

	do {
		hList = NULL;

		entry = NULL;
		parent = (XplDirMatch *)calloc(1, sizeof(XplDirMatch));
		if (parent) {
			if (pattern && ((end = strrchr(pattern, '/')) != NULL)) {
				end++;

				count = end - pattern;
				memcpy(path, pattern, count);
			} else {
				count = 2;
				memcpy(path, "./", count);
			}

			end = path + count;
			*end = '\0';
		} else {
			break;
		}

		parent->d_name = strdup(path);
		if (parent->d_name) {
			hList = FindFirstFile(pattern, &hData);
		} else {
			break;
		}

		if (hList != INVALID_HANDLE_VALUE) {
			finished = FALSE;
		} else {
			return(parent);
		}

		while (!finished) {
			if (!(hData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if (entry == NULL) {
					entry = (XplDirMatch *)calloc(1, sizeof(XplDirMatch));
				}

				if (entry != NULL) {
					entry->d_name = strdup(hData.cFileName);
					if (entry->d_name) {
#if defined(DEBUG)
						struct stat sb;
#endif

						entry->d_size = hData.nFileSizeLow;
						entry->d_cdatetime = (unsigned long)(((((ULONGLONG)hData.ftLastWriteTime.dwHighDateTime << 32) | hData.ftLastWriteTime.dwLowDateTime) - 116444736000000000) / 10000000);
						entry->d_attr = hData.dwFileAttributes;

						entry->base = parent;
						entry->next = parent->next;
						parent->next = entry;

#if defined(DEBUG)
						{
							strcpy(end, entry->d_name);
							if (stat(path, &sb) == 0) {
								if (sb.st_mtime != entry->d_cdatetime) {
									XplConsolePrintf("NMAPQ: DEBUG Time drift detected on \"%s\"\r\n\tstat is %d\r\n\tfiletime is %d\r\n\tdrift is %d.\r\n", path, sb.st_mtime, entry->d_cdatetime, max(sb.st_mtime, entry->d_cdatetime) - min(sb.st_mtime, entry->d_cdatetime));
								}
							} else {
								XplConsolePrintf("NMAPQ: Unable to stat \"%s\"; errno %d.\r\n", path, errno);
							}
						}
#endif

						entry = NULL;
					} else {
						break;
					}
				} else {
					break;
				}
			}

			if (FindNextFile(hList, &hData)) {
				continue;
			}

			if (GetLastError() == ERROR_NO_MORE_FILES) {
				finished = TRUE;

				continue;
			}

			break;
		}

		if (finished) {
			parent->base = parent->next;

			return(parent);
		}

		break;
	} while (FALSE);

	if (hList) {
		FindClose(hList);
	}

	if (entry) {
		if (entry->d_name) {
			free(entry->d_name);
		}

		free(entry);
	}

	while (parent) {
		entry = parent->next;

		if (parent->d_name) {
			free(parent->d_name);
		}

		free(parent);

		parent = entry;
	}

	return(NULL);
}



#else /* defined(WIN32) */

#error XplOpenDirMatch not implemented on this platform.

#endif

EXPORT XplDirMatch * XplReadDirMatch(XplDirMatch *dirp)
{
	XplDirMatch *entry;

	entry = dirp->next;
	if (entry) {
		dirp->next = entry->next;
	}

	return(entry);
}

EXPORT int XplCloseDirMatch(XplDirMatch *dirp)
{
	XplDirMatch *next;
	XplDirMatch *entry;

	entry = dirp->base;
	while (entry) {
		next = entry->next;

		if (entry->d_name) {
			free(entry->d_name);
		}

		free(entry);
		entry = next;
	}

	free(dirp->d_name);
	free(dirp);
	return(0);
}


#ifdef WIN32
#ifndef __WATCOMC__
struct dirent
{
	char d_name[256];
};

struct vcdir
{
	HANDLE hand;
	int inUse;
	struct dirent entry;
	WIN32_FIND_DATA data;
};

struct vcdir *opendir( char *name )
{
	struct vcdir *dir;

	if( ( dir = (struct vcdir *)MemMalloc( sizeof( struct vcdir ) ) ) )
	{
		memset( dir, 0, sizeof( struct vcdir ) );
		if( ( dir->hand = FindFirstFile( name, &dir->data ) ) )
		{
			dir->inUse = 1;
			return dir;
		}
		MemFree( dir );
		return NULL;
	}

	return NULL;
}

struct dirent *readdir( struct vcdir *dir )
{
	if( dir->inUse )
	{
		strncpy( dir->entry.d_name, dir->data.cFileName, sizeof( dir->entry.d_name ) );
		if( !FindNextFile( dir->hand, &dir->data ) )
		{
			dir->inUse = 0;
		}
		return &dir->entry;
	}
	return NULL;
}

int closedir( struct vcdir *dir )
{
	if( dir )
	{
		FindClose( dir->hand );
		MemFree( dir );
		return 0;
	}
	return EINVAL;
}

#endif
#endif

EXPORT XDir *XOpenDir( const char *path, void *(*XAlloc)(size_t), void (*XFree)(void *) )
{
	XDir	*dir;

	if( ( dir = (XDir *)XAlloc( sizeof( XDir ) +  strlen( path ) + 2 ) ) )
	{
		memset( dir, 0, sizeof( XDir ) );
		dir->XFree = XFree;
		strcpy( dir->name, path );
		if( dir->pattern = strrchr( dir->name, '/' ) )
		{
			*dir->pattern = '\0';
			dir->pattern++;
			if( !*dir->pattern )
			{
				dir->pattern = NULL;
			}
		}
		if( ( dir->dirp = opendir( dir->name ) ) )
		{
			return dir;
		}
		XFree( dir );
	}
	return NULL;
}

EXPORT XDirEnt *XReadDir( XDir *dir )
{
	struct	dirent *entry;
	char	fullPath[1024];

	if( dir )
	{
		while( ( entry = readdir( dir->dirp ) ) )
		{
			if( !strcmp( entry->d_name, "." ) || !strcmp( entry->d_name, ".." ) )
			{
				continue;
			}
			if( !dir->pattern || !strpat( entry->d_name, dir->pattern ) )
			{
				sprintf( fullPath, "%s/%s", dir->name, entry->d_name );
				stat( fullPath, &dir->entry.st );
				strcpy( dir->entry.d_name, entry->d_name );
				return &dir->entry;
			}
		}
	}
	return NULL;
}

EXPORT int XCloseDir( XDir *dir )
{
	void	*dirp;

	if( dir )
	{
		dirp = dir->dirp;
		dir->XFree( dir );
		return closedir( dirp );
	}
	return errno = EINVAL;
}

EXPORT void XplMakePath(const char *path)
{
	char	*slash;
	char	*next;
	char	p[XPL_MAX_PATH + 1] = "";

	strprintf(p, sizeof(p), NULL, "%s", path);

	if (!(slash = strchr(p, '/'))) {
		slash = strchr(p, '\\');
	}

	while (slash) {
		*slash = '\0';
		XplMakeDir(p);
		*slash = '/';

		if (!(next = strchr(slash + 1, '/'))) {
			next = strchr(slash + 1, '\\');
		}

		slash = next;
	}

	XplMakeDir(p);
}
