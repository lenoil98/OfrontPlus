MODULE Platform; (** Unix *)
IMPORT SYSTEM;

(* Based on Vishap Oberon (voc) runtime by David C W Brown, 2016-2018
   Improvements by Oleg N. Cher, 2019-2020 and Arthur Yefimov, 2022
*)


CONST
  PathDelimiter* = "/";
  PathSeparator* = ":";

  Unix*    = TRUE;
  Windows* = FALSE;
  
  StdIn*  = 0;
  StdOut* = 1;
  StdErr* = 2;

  NewLine* = 0AX + 0X;  (* Platform specific new line representation *)


TYPE
  CHAR = SHORTCHAR;
  ADRINT* = SYSTEM.ADRINT;  (* 32 or 64 bits *)
  
  TIME_T = ADRINT;  (* time_t type is used
    instead of long, but which is also a 32-bit representation on some systems
    while there are even 64-bit representations on other systems. *)
  DEV_T = LONGINT;  (* 64 bit in Linux 2.2 *)
  INO_T = ADRINT;  (* 32 or 64 bits *)

  ErrorCode*  = INTEGER;
  FileHandle* = INTEGER;  (* In UNIX, a file handle is an opaque number
    that is used to uniquely identify a file or other file system object.
    The only operations that can be carried out with the file handle in UNIX
    are to copy and compare it for equality with another file handle. *)

  FileIdentity* = RECORD [notag]
    volume: DEV_T;   (* dev on Unix filesystems, volume serial number on NTFS *)
    index:  INO_T;   (* inode on Unix filesystems, file id on NTFS *)
    mtime:  TIME_T;  (* File modification time, value is system dependent *)
  END;

  EnvPtr  = POINTER [notag] TO ARRAY 1024 OF CHAR;


VAR
  LittleEndian-:   BOOLEAN;
  PID-:            INTEGER;  (* Note: Must be updated by Fork implementation *)
  CWD-:            ARRAY 4096 OF CHAR;

  SeekSet-:        INTEGER;
  SeekCur-:        INTEGER;
  SeekEnd-:        INTEGER;


(* Unix headers to be included *)

PROCEDURE- AAincludeSystime  "#include <sys/time.h>";  (* for gettimeofday() *)
PROCEDURE- AAincludeTime     "#include <time.h>";      (* for localtime() *)
PROCEDURE- AAincludeSystypes "#include <sys/types.h>";
PROCEDURE- AAincludeUnistd   "#include <unistd.h>";
PROCEDURE- AAincludeSysstat  "#include <sys/stat.h>";
PROCEDURE- AAincludeFcntl    "#include <fcntl.h>";
PROCEDURE- AAincludeErrno    "#include <errno.h>";
PROCEDURE- AAincludeStdlib   "#include <stdlib.h>";
PROCEDURE- AAincludeStdio    "#include <stdio.h>";
PROCEDURE- AAincludeLimits   "#include <limits.h>";


(* Error code tests *)

PROCEDURE- EMFILE():       ErrorCode "EMFILE";
PROCEDURE- ENFILE():       ErrorCode "ENFILE";
PROCEDURE- ENOENT():       ErrorCode "ENOENT";
PROCEDURE- EXDEV():        ErrorCode "EXDEV";
PROCEDURE- EACCES():       ErrorCode "EACCES";
PROCEDURE- EROFS():        ErrorCode "EROFS";
PROCEDURE- EAGAIN():       ErrorCode "EAGAIN";
PROCEDURE- ETIMEDOUT():    ErrorCode "ETIMEDOUT";
PROCEDURE- ECONNREFUSED(): ErrorCode "ECONNREFUSED";
PROCEDURE- ECONNABORTED(): ErrorCode "ECONNABORTED";
PROCEDURE- ENETUNREACH():  ErrorCode "ENETUNREACH";
PROCEDURE- EHOSTUNREACH(): ErrorCode "EHOSTUNREACH";


PROCEDURE TooManyFiles* (e: ErrorCode): BOOLEAN;
BEGIN RETURN (e = EMFILE()) OR (e = ENFILE()) END TooManyFiles;

PROCEDURE NoSuchDirectory* (e: ErrorCode): BOOLEAN;
BEGIN RETURN e = ENOENT() END NoSuchDirectory;

PROCEDURE DifferentFilesystems* (e: ErrorCode): BOOLEAN;
BEGIN RETURN e = EXDEV() END DifferentFilesystems;

PROCEDURE Inaccessible* (e: ErrorCode): BOOLEAN;
BEGIN RETURN (e = EACCES()) OR (e = EROFS()) OR (e = EAGAIN()) END Inaccessible;

PROCEDURE Absent* (e: ErrorCode): BOOLEAN;
BEGIN RETURN (e = ENOENT()) END Absent;

PROCEDURE TimedOut* (e: ErrorCode): BOOLEAN;
BEGIN RETURN (e = ETIMEDOUT()) END TimedOut;

PROCEDURE ConnectionFailed* (e: ErrorCode): BOOLEAN;
BEGIN RETURN (e = ECONNREFUSED()) OR (e = ECONNABORTED())
          OR (e = ENETUNREACH())  OR (e = EHOSTUNREACH()) END ConnectionFailed;


(* OS memory allocaton *)

PROCEDURE- allocate (size: ADRINT): ADRINT "((SYSTEM_ADRINT)malloc((size_t)(size)))";

PROCEDURE- SystemHalt (code: INTEGER; mod: ARRAY OF SHORTCHAR; pos: INTEGER) "__HALT(code, mod, pos)";

PROCEDURE OSAllocate* (size: ADRINT): ADRINT;
BEGIN
  IF size > 0 THEN
    RETURN allocate(size)  (* malloc will return the null pointer when it fails. *)
  END;
  SystemHalt(-25, "Platform.Mod", 128); RETURN 0 (* To remove __RETCHK *)
END OSAllocate;

PROCEDURE- free (address: ADRINT) "free((void*)(address))";
PROCEDURE OSFree* (address: ADRINT); BEGIN free(address) END OSFree;


(* Program arguments and environment access *)

PROCEDURE- getenv (var: ARRAY OF CHAR): EnvPtr "(Platform_EnvPtr)getenv((char*)var)";

PROCEDURE getEnv (IN var: ARRAY OF CHAR; VAR val: ARRAY OF CHAR): BOOLEAN;
VAR p: EnvPtr;
BEGIN
  p := getenv(var);
  IF p # NIL THEN val := p^$ END;
  RETURN p # NIL
END getEnv;

PROCEDURE GetEnv* (IN var: ARRAY OF CHAR; OUT val: ARRAY OF CHAR);
BEGIN
  IF ~getEnv(var, val) THEN val := "" END
END GetEnv;


(* Time of day *)

(* struct timeval
   The struct timeval structure represents an elapsed time. It is declared
   in sys/time.h and has the following members:

      time_t tv_sec
      This represents the number of whole seconds of elapsed time.

      suseconds_t tv_usec
      This is the rest of the elapsed time (a fraction of a second), represented
      as the number of microseconds. It is always less than one million.

   Using long int for tv_sec will work until year 2038, and after that the
   tv_sec will overflow on machines where long is 4 bytes.
    
   That is why some are using unsigned long, as it will stop the overflow
   until year 2100+. You should use the time_t type instead, and you won't need
   to think about how long your program is supposed to run for in the future.
*)
PROCEDURE- gettimeval          "struct timeval tv; gettimeofday(&tv, 0)";
PROCEDURE- tvsec ():  TIME_T   "tv.tv_sec";
PROCEDURE- tvusec (): INTEGER  "(INTEGER)tv.tv_usec";
PROCEDURE- sectotm (s: TIME_T) "struct tm *time = localtime((time_t*)&s)";
PROCEDURE- tmsec ():  INTEGER  "time->tm_sec";
PROCEDURE- tmmin ():  INTEGER  "time->tm_min";
PROCEDURE- tmhour (): INTEGER  "time->tm_hour";
PROCEDURE- tmmday (): INTEGER  "time->tm_mday";
PROCEDURE- tmmon ():  INTEGER  "time->tm_mon";
PROCEDURE- tmyear (): INTEGER  "time->tm_year";

PROCEDURE YMDHMStoClock (ye, mo, da, ho, mi, se: INTEGER; VAR t, d: INTEGER);
BEGIN
  d := ASH(ye MOD 100, 9) + ASH(mo+1, 5) + da;
  t := ASH(ho, 12)        + ASH(mi, 6)   + se
END YMDHMStoClock;

PROCEDURE GetClock* (VAR t, d: INTEGER);
BEGIN
  gettimeval; sectotm(tvsec());
  YMDHMStoClock(tmyear(), tmmon(), tmmday(), tmhour(), tmmin(), tmsec(), t, d)
END GetClock;

PROCEDURE- timeint (time: TIME_T): INTEGER "((INTEGER)(time))";

PROCEDURE Time* (): INTEGER;
BEGIN
  gettimeval;
  RETURN (tvusec() DIV 1000 + timeint(tvsec()) * 1000) MOD 7FFFFFFFH
END Time;

(* struct timespec
   Structure holding an interval broken down into seconds and nanoseconds.

      time_t tv_sec  whole seconds (valid values are >= 0)
      long tv_nsec   nanoseconds (valid values are [0, 999999999])
*)
PROCEDURE- nanosleep (s, ns: INTEGER) "struct timespec req, rem; req.tv_sec = (time_t)s; req.tv_nsec = ns; nanosleep(&req, &rem)";

PROCEDURE Delay* (ms: INTEGER);
VAR s, ns: INTEGER;
BEGIN
  s  :=  ms DIV 1000;
  ns := (ms MOD 1000) * 1000000;
  nanosleep(s, ns)
END Delay;


(* System call *)

PROCEDURE- system (str: ARRAY OF CHAR): INTEGER "system((char*)str)";
PROCEDURE- err (): INTEGER "errno";

PROCEDURE System* (IN cmd : ARRAY OF CHAR): INTEGER;
BEGIN RETURN system(cmd) END System;

PROCEDURE Error* (): ErrorCode; BEGIN RETURN err() END Error;


(* Expose file and path name length limits *)

PROCEDURE- NAMEMAX (): INTEGER "NAME_MAX";
PROCEDURE- PATHMAX (): INTEGER "PATH_MAX";

PROCEDURE MaxNameLength* (): INTEGER; BEGIN RETURN NAMEMAX() END MaxNameLength;
PROCEDURE MaxPathLength* (): INTEGER; BEGIN RETURN PATHMAX() END MaxPathLength; 


(* File system *)

PROCEDURE- InvalidHandleValue* (): FileHandle "(-1)";

(* Note: Consider also using flags O_SYNC and O_DIRECT as we do buffering *)
PROCEDURE- openrw (n: ARRAY OF CHAR): FileHandle "open((char*)n, O_RDWR)";
PROCEDURE- openro (n: ARRAY OF CHAR): FileHandle "open((char*)n, O_RDONLY)";
PROCEDURE- opennew (n: ARRAY OF CHAR): FileHandle "open((char*)n, O_CREAT | O_TRUNC | O_RDWR, 0664)";
PROCEDURE- mkdir (pathname: ARRAY OF CHAR): INTEGER "mkdir((char*)pathname, 0775)";


(* File APIs *)

PROCEDURE OldRO* (IN pathname: ARRAY OF CHAR; VAR h: FileHandle): ErrorCode;
VAR fd: FileHandle;
BEGIN
  fd := openro(pathname);
  IF fd = InvalidHandleValue() THEN RETURN err() ELSE h := fd; RETURN 0 END
END OldRO;

PROCEDURE OldRW* (IN pathname: ARRAY OF CHAR; VAR h: FileHandle): ErrorCode;
VAR fd: FileHandle;
BEGIN
  fd := openrw(pathname);
  IF fd = InvalidHandleValue() THEN RETURN err() ELSE h := fd; RETURN 0 END
END OldRW;

PROCEDURE NewFile* (IN pathname: ARRAY OF CHAR; VAR h: FileHandle): ErrorCode;
VAR fd: FileHandle;
BEGIN
  fd := opennew(pathname);
  IF fd = InvalidHandleValue() THEN RETURN err() ELSE h := fd; RETURN 0 END
END NewFile;

PROCEDURE NewDir* (IN pathname: ARRAY OF CHAR; VAR h: FileHandle): ErrorCode;
BEGIN
  IF mkdir(pathname) # 0 THEN RETURN err() ELSE RETURN 0 END
END NewDir;


PROCEDURE- closefile (fd: FileHandle): INTEGER "close(fd)";

PROCEDURE CloseFile* (handle: FileHandle): ErrorCode;
BEGIN
  IF closefile(handle) < 0 THEN RETURN err() ELSE RETURN 0 END
END CloseFile;


PROCEDURE- fstat (fd: FileHandle):  INTEGER "fstat(fd, &s)";
PROCEDURE- stat (n: ARRAY OF CHAR): INTEGER "stat((char*)n, &s)";
PROCEDURE- structstats                      "struct stat s";
PROCEDURE- statdev ():              DEV_T   "(LONGINT)s.st_dev";
PROCEDURE- statino ():              INO_T   "(SYSTEM_ADRINT)s.st_ino";
PROCEDURE- statmtime ():            TIME_T  "(SYSTEM_ADRINT)s.st_mtime";
PROCEDURE- statsize ():             LONGINT "(LONGINT)s.st_size";
PROCEDURE- statisreg ():            BOOLEAN "S_ISREG(s.st_mode)";
PROCEDURE- statisdir ():            BOOLEAN "S_ISDIR(s.st_mode)";

PROCEDURE Identify* (h: FileHandle; VAR identity: FileIdentity): ErrorCode;
BEGIN
  structstats;
  IF fstat(h) < 0 THEN RETURN err() END;
  identity.volume := statdev();
  identity.index  := statino();
  identity.mtime  := statmtime();
  RETURN 0
END Identify;

PROCEDURE IdentifyByName* (IN n: ARRAY OF CHAR; VAR identity: FileIdentity): ErrorCode;
BEGIN
  structstats;
  IF stat(n) < 0 THEN RETURN err() END;
  identity.volume := statdev();
  identity.index  := statino();
  identity.mtime  := statmtime();
  RETURN 0
END IdentifyByName;


PROCEDURE SameFile* (i1, i2: FileIdentity): BOOLEAN;
BEGIN RETURN (i1.index = i2.index) & (i1.volume = i2.volume)
END SameFile;

PROCEDURE SameFileTime* (i1, i2: FileIdentity): BOOLEAN;
BEGIN RETURN i1.mtime = i2.mtime
END SameFileTime;

PROCEDURE SetMTime* (VAR target: FileIdentity; source: FileIdentity);
BEGIN target.mtime := source.mtime
END SetMTime;

PROCEDURE MTimeAsClock* (i: FileIdentity; VAR t, d: INTEGER);
BEGIN
  sectotm(i.mtime);
  YMDHMStoClock(tmyear(), tmmon(), tmmday(), tmhour(), tmmin(), tmsec(), t, d)
END MTimeAsClock;


PROCEDURE FileSize* (h: FileHandle; VAR len: LONGINT): ErrorCode;
BEGIN
  structstats;
  IF fstat(h) < 0 THEN RETURN err() END;
  len := statsize();
  RETURN 0
END FileSize;

(* Returns path for temporary files specified by the current OS. Path will end
   with a path delimiter. If the parameter path has no space for the whole
   path, the default /tmp/ path is used, empty string is never returned. *)
PROCEDURE GetTempPath* (OUT path: ARRAY OF CHAR);
VAR s: ARRAY 1024 OF CHAR;
  i: INTEGER;
BEGIN
  GetEnv('TMPDIR', s);
  i := 0; WHILE s[i] # 0X DO INC(i) END;
  IF (i # 0) & (s[i - 1] # PathDelimiter) THEN
    s[i] := PathDelimiter; INC(i); s[i] := 0X
  END;
  IF (i # 0) & (i < LEN(path)) THEN path := s$ ELSE path := '/tmp/' END
END GetTempPath;

PROCEDURE- readfile (fd: FileHandle; p: ADRINT; l: INTEGER): INTEGER
"read(fd, (void*)(p), l)";

PROCEDURE Read* (h: FileHandle; p: ADRINT; l: INTEGER; VAR n: INTEGER): ErrorCode;
BEGIN
  n := readfile(h, p, l);
  IF n < 0 THEN n := 0; RETURN err() ELSE RETURN 0 END
END Read;

PROCEDURE ReadBuf* (h: FileHandle; VAR b: ARRAY OF BYTE; VAR n: INTEGER): ErrorCode;
BEGIN
  n := readfile(h, SYSTEM.ADR(b), LEN(b));
  IF n < 0 THEN n := 0; RETURN err() ELSE RETURN 0 END
END ReadBuf;


PROCEDURE- writefile (fd: FileHandle; p: ADRINT; l: INTEGER): INTEGER
"write(fd, (void*)(p), l)";

PROCEDURE Write* (h: FileHandle; p: ADRINT; l: INTEGER): ErrorCode;
VAR written: INTEGER;
BEGIN
  written := writefile(h, p, l);
  IF written < 0 THEN RETURN err() ELSE RETURN 0 END
END Write;


PROCEDURE- fsync (fd: FileHandle): INTEGER "fsync(fd)";

PROCEDURE Sync* (h: FileHandle): ErrorCode;
BEGIN
  IF fsync(h) < 0 THEN RETURN err() ELSE RETURN 0 END
END Sync;


PROCEDURE- lseek (fd: FileHandle; offset: LONGINT; whence: INTEGER): INTEGER "lseek(fd, offset, whence)";
PROCEDURE- seekset (): INTEGER "SEEK_SET";
PROCEDURE- seekcur (): INTEGER "SEEK_CUR";
PROCEDURE- seekend (): INTEGER "SEEK_END";

PROCEDURE Seek* (h: FileHandle; offset: LONGINT; whence: INTEGER): ErrorCode;
BEGIN
  IF lseek(h, offset, whence) < 0 THEN RETURN err() ELSE RETURN 0 END
END Seek;


PROCEDURE- ftruncate (fd: FileHandle; length: LONGINT): INTEGER "ftruncate(fd, length)";

PROCEDURE TruncateFile* (h: FileHandle; limit: LONGINT): ErrorCode;
BEGIN
  IF ftruncate(h, limit) < 0 THEN RETURN err() ELSE RETURN 0 END
END TruncateFile;


PROCEDURE- unlink (n: ARRAY OF CHAR): INTEGER "unlink((char*)n)";

PROCEDURE DeleteFile* (IN n: ARRAY OF CHAR): ErrorCode;
BEGIN
  IF unlink(n) < 0 THEN RETURN err() ELSE RETURN 0 END
END DeleteFile;


PROCEDURE- chdir (n: ARRAY OF CHAR): INTEGER "chdir((char*)n)";
PROCEDURE- getcwd (VAR cwd: ARRAY OF CHAR) "{char *dummy = getcwd((char*)cwd, cwd__len);}";

PROCEDURE ChDir* (IN n: ARRAY OF CHAR): ErrorCode;
VAR r: INTEGER;
BEGIN
  r := chdir(n);  getcwd(CWD);
  IF r < 0 THEN RETURN err() ELSE RETURN 0 END
END ChDir;


PROCEDURE FileExists* (IN pathname: ARRAY OF CHAR): BOOLEAN;
BEGIN
  structstats;
  IF stat(pathname) # 0 THEN RETURN FALSE END;
  RETURN statisreg()
END FileExists;

PROCEDURE DirExists* (IN path: ARRAY OF CHAR): BOOLEAN;
BEGIN
  structstats;
  IF stat(path) # 0 THEN RETURN FALSE END;
  RETURN statisdir()
END DirExists;


PROCEDURE- rename (o, n: ARRAY OF CHAR): INTEGER "rename((char*)o, (char*)n)";

PROCEDURE CopyFileFd (from, to: FileHandle): ErrorCode;
VAR buf: ARRAY 32768 OF BYTE;
  n: INTEGER;
  err: ErrorCode;
BEGIN
  err := ReadBuf(from, buf, n);
  WHILE (err = 0) & (n > 0) DO
    err := Write(to, SYSTEM.ADR(buf), n);
    IF err = 0 THEN err := ReadBuf(from, buf, n) END
  END;
  RETURN err
END CopyFileFd;

PROCEDURE CopyFile (IN oldname, newname: ARRAY OF CHAR): ErrorCode;
VAR from, to: FileHandle;
  res: ErrorCode;
BEGIN
  from := openro(oldname);
  IF from # -1 THEN
    to := opennew(newname);
    IF to # -1 THEN
      res := CopyFileFd(from, to);
      IF closefile(to) = -1 THEN res := err() END
    ELSE res := -101
    END;
    IF closefile(from) = -1 THEN res := err() END
  ELSE res := -102
  END;
  RETURN res
END CopyFile;

PROCEDURE RenameFile* (IN oldname, newname: ARRAY OF CHAR): ErrorCode;
VAR res: ErrorCode;
BEGIN
  res := rename(oldname, newname);
  IF res < 0 THEN
    res := err();
    IF res = EXDEV() THEN
      res := CopyFile(oldname, newname)
    END
  END;
  RETURN res
END RenameFile;


(* Process termination *)

PROCEDURE- exit (code: INTEGER) "exit(code)";
PROCEDURE ExitOS* (code: INTEGER);
BEGIN exit(code) END ExitOS;


PROCEDURE- isatty (fd: FileHandle): INTEGER "isatty(fd)";

PROCEDURE IsConsole* (h: FileHandle): BOOLEAN;
BEGIN RETURN isatty(h) # 0 END IsConsole;


PROCEDURE TestLittleEndian;
VAR i: INTEGER;
BEGIN i := 1; SYSTEM.GET(SYSTEM.ADR(i), LittleEndian) END TestLittleEndian;


PROCEDURE- getpid (): INTEGER "(INTEGER)getpid()";

BEGIN
  TestLittleEndian;
  CWD := ""; getcwd(CWD);
  PID := getpid();
  SeekSet := seekset();
  SeekCur := seekcur();
  SeekEnd := seekend()
END Platform.
