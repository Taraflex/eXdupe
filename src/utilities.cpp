// SPDX-License-Identifier: GPL-2.0-or-later
//
// eXdupe deduplication library and file archiver.
//
// Copyrights:
// 2010 - 2024: Lasse Mikkel Reinhold

#include <filesystem>
#include <iostream>
#include <random>
#include <time.h>
#include <iomanip>
#include <cmath>
#include <cfenv>

#include "unicode.h"
#include "utilities.hpp"

#ifdef WINDOWS
#include "Shlwapi.h"
#else
unsigned int GetTickCount() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#endif

#ifdef WINDOWS
#define CURDIR UNITXT(".\\")
#define DELIM_STR UNITXT("\\")
#define DELIM_CHAR '\\'
#else
#define CURDIR UNITXT("./")
#define DELIM_STR UNITXT("/")
#define DELIM_CHAR UNITXT('/')
#endif

std::string format_size(uint64_t size) {
    if (size <= 999) {
        return std::to_string(size) + " B";
    }

    const char *suffixes[] = {" B", " KB", " MB", " GB", " TB", " PB" };
    int suffixIndex = 0;

    double sizeInKB = static_cast<double>(size);

    while (sizeInKB >= 1024.0 && suffixIndex < 8) {
        sizeInKB /= 1024.0;
        suffixIndex++;
    }

    if (sizeInKB >= 1000.0) {
        sizeInKB /= 1024.0;
        suffixIndex++;
    }

    std::ostringstream oss;
    const auto prev_round = std::fegetround();
    std::fesetround(FE_DOWNWARD);  

    if (sizeInKB > 999) {
        oss << std::fixed << std::setprecision(0);
    } else if (sizeInKB > 99) {
        oss << std::fixed << std::setprecision(0);
    } else if (sizeInKB > 9.9) {
        oss << std::fixed << std::setprecision(1);
    } else {
        oss << std::fixed << std::setprecision(2);
    }
    oss << sizeInKB << "" << suffixes[suffixIndex];

    string ret = oss.str();
    return ret;
}

STRING s2w(const std::string &s) { return STRING(s.begin(), s.end()); }

STRING left(const STRING &s) {
    const size_t t = s.find_last_of(UNITXT("/\\"));
    if (t != string::npos) {
        return s.substr(0, t);
    } else {
        return UNITXT("");
    }
}

STRING right(const STRING &s) {
    const size_t t = s.find_last_of(UNITXT("/\\"));
    if (t != string::npos) {
        return s.substr(t + 1);
    } else {
        return UNITXT("");
    }
}

uint64_t rnd64() {
    std::random_device rd;
    std::mt19937_64 eng(rd());
    std::uniform_int_distribution<uint64_t> distr;
    return distr(eng);
}

string wstring2string(STRING wstr) {
    string str(wstr.length(), ' ');
    copy(wstr.begin(), wstr.end(), str.begin());
    return str;
}

STRING string2wstring(string str) {
    STRING wstr(str.length(), L' ');
    copy(str.begin(), str.end(), wstr.begin());
    return wstr;
}

void myReplace(std::STRING &str, const std::STRING &oldStr, const std::STRING &newStr) {
    size_t pos = 0;
    while ((pos = str.find(oldStr, pos)) != std::STRING::npos) {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

void myReplaceSTR(std::string &str, const std::string &oldStr, const std::string &newStr) {
    size_t pos = 0;
    while ((pos = str.find(oldStr, pos)) != std::STRING::npos) {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

void set_date(STRING file, tm *tm_date) {
#ifdef WINDOWS
    SYSTEMTIME myTime;
    FILETIME ft;

    myTime.wYear = (WORD)tm_date->tm_year;
    myTime.wMonth = (WORD)tm_date->tm_mon;
    myTime.wSecond = (WORD)tm_date->tm_sec;
    myTime.wHour = (WORD)tm_date->tm_hour;
    myTime.wMinute = (WORD)tm_date->tm_min;
    myTime.wDay = (WORD)tm_date->tm_mday;
    myTime.wDayOfWeek = (WORD)tm_date->tm_wday;
    myTime.wMilliseconds = 0;

    bool b = (bool)SystemTimeToFileTime(&myTime, &ft);
    HANDLE hFile;
    hFile = CreateFileW(file.c_str(), FILE_GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    b = (bool)SetFileTime(hFile, &ft, &ft, &ft);
    CloseHandle(hFile);
#else
    struct utimbuf Time;
    tm_date->tm_year -= 1900;
    time_t t = mktime(tm_date);
    Time.actime = t;
    Time.modtime = t;
    utime(file.c_str(), &Time);
#endif
}

bool is_symlink(STRING file) { return ISLINK(get_attributes(file, false)); }

bool is_named_pipe(STRING file) { return ISNAMEDPIPE(get_attributes(file, false)); }

void cur_date(tm *tm_date) {
#ifdef WINDOWS
    SYSTEMTIME myTime;
    GetSystemTime(&myTime);
    tm_date->tm_hour = myTime.wHour;
    tm_date->tm_min = myTime.wMinute;
    tm_date->tm_mday = myTime.wDay;
    tm_date->tm_mon = myTime.wMonth;
    tm_date->tm_sec = myTime.wSecond;
    tm_date->tm_year = myTime.wYear;
    tm_date->tm_wday = myTime.wDayOfWeek;
#else
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    timeinfo->tm_year += 1900;
    memcpy(tm_date, timeinfo, sizeof(tm));
#endif
}

void get_date(STRING file, tm *tm_date) {
#ifdef WINDOWS
    SYSTEMTIME myTime;

    HANDLE hFind;
    WIN32_FIND_DATAW FindData;

    hFind = FindFirstFileW(file.c_str(), &FindData);

    if (hFind != INVALID_HANDLE_VALUE) {
        FileTimeToSystemTime(&FindData.ftLastWriteTime, &myTime);
        FindClose(hFind);
        tm_date->tm_hour = myTime.wHour;
        tm_date->tm_min = myTime.wMinute;
        tm_date->tm_mday = myTime.wDay;
        tm_date->tm_mon = myTime.wMonth;
        tm_date->tm_sec = myTime.wSecond;
        tm_date->tm_year = myTime.wYear;
        tm_date->tm_wday = myTime.wDayOfWeek;
    } else {
        tm_date->tm_hour = 0;
        tm_date->tm_min = 0;
        tm_date->tm_mday = 1;
        tm_date->tm_mon = 1;
        tm_date->tm_sec = 0;
        tm_date->tm_year = 1970;
        tm_date->tm_wday = 0;
    }

#else
    struct tm *clock;
    struct stat attrib;
    stat(file.c_str(), &attrib);
    clock = gmtime(&(attrib.st_mtime));
    clock->tm_year += 1900;
    memcpy(tm_date, clock, sizeof(tm));

#endif
}

STRING abs_path(STRING source) {
    CHR destination[5000];
    CHR *r;
#ifdef WINDOWS
    r = _wfullpath(destination, source.c_str(), 5000);
    return r == 0 ? UNITXT("") : destination;
#else
    if (fs::is_symlink(source)) {
        fs::path p(source);
        auto parent = p.parent_path();
        if (parent == "") {
            parent = ".";
        }

        fs::path absolutePath = fs::canonical(parent);
        auto res = absolutePath / p.filename();
        return res;
    }

    r = realpath(source.c_str(), destination);
    return r == 0 ? UNITXT("") : destination;
#endif
}

STRING slashify(STRING path) {
#ifdef WINDOWS
    replace(path.begin(), path.end(), '/', '\\');
    return path;
#else
    replace(path.begin(), path.end(), '\\', '/');
    return path;
#endif
}

STRING ucase(STRING str) {
    // change each element of the STRING to upper case
    for (unsigned int i = 0; i < str.length(); i++) {
        str[i] = (char)toupper(str[i]);
    }
    return str;
}

STRING lcase(STRING str) {
    // change each element of the STRING to lower case
    STRING s = str;
    for (unsigned int i = 0; i < s.length(); i++) {
        s[i] = (char)tolower(s[i]);
    }
    return s;
}

/* reverse:  reverse STRING s in place */
void reverse(char s[]) {
    size_t j;
    unsigned char c;
    unsigned int i;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* itoa:  convert n to characters in s */
void itoa(int n, char s[]) {
    int i, sign;

    if ((sign = n) < 0) { /* record sign */
        n = -n;           /* make n positive */
    }
    i = 0;
    do {                       /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

void checksum_init(checksum_t *t) {
    t->remainder = 0;
    t->remainder_len = 0;
    t->b_val = 0x794e80091e8f2bc7ULL;
    t->a_val = 0xc20f9a8b761b7e4cULL;
    t->result = 0;
}

void checksum(unsigned char *data, size_t len, checksum_t *t) {
    if (len == 0) {
        return;
    }

    while (t->remainder_len < 8 && len > 0) {
        t->remainder = t->remainder >> 8;
        t->remainder = t->remainder | (uint64_t)*data << (7 * 8);
        t->remainder_len++;
        data++;
        len--;
    }

    if (t->remainder_len < 8) {
        t->result = t->a_val + t->b_val + t->remainder + t->remainder_len;
        return;
    }

    t->a_val += t->remainder * t->b_val;
    t->b_val++;
    t->remainder_len = 0;
    t->remainder = 0;

    {
        uint64_t a = t->a_val;
        uint64_t b = t->b_val;
        auto last_data = data + ((len / 8) * 8);
        len -= ((len / 8) * 8);
        while (data < last_data) {
#ifdef X86X64
            a += (*(uint64_t *)data) * b;
#else
            uint64_t l = 0;
            for (unsigned int i = 0; i < 8; i++) {
                l = l >> 8;
                l = l | (uint64_t) * (data + i) << (7 * 8);
            }
            a += l * b;
#endif
            b++;
            data += 8;
        }
        t->a_val = a;
        t->b_val = b;
    }

    while (len > 0) {
        t->remainder = t->remainder >> 8;
        t->remainder = t->remainder | (uint64_t)*data << (7 * 8);
        t->remainder_len++;
        data++;
        len--;
    }
    t->result = t->a_val + t->b_val + t->remainder + t->remainder_len;

    return;
}

uint64_t filesize(STRING file, bool followlinks = false) {
#ifndef WINDOWS
    struct stat buf;
    int i;

    if (followlinks) {
        i = stat(file.c_str(), &buf);
    } else {
        i = lstat(file.c_str(), &buf);
    }

    if (i == 0) {
        return buf.st_size;
    } else {
        return 0;
    }
#else
    HANDLE hFind;
    WIN32_FIND_DATAW FindData;
    (void)followlinks;
    hFind = FindFirstFileW(file.c_str(), &FindData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return (uint64_t)-1;
    } else {
        FindClose(hFind);
        return (FindData.nFileSizeHigh * (static_cast<uint64_t>(MAXDWORD) + 1)) + FindData.nFileSizeLow;
    }
#endif
}

bool exists(STRING file) {
#ifndef WINDOWS
    struct stat buf;
    int i = stat(file.c_str(), &buf);
    if (i == 0) {
        return true;
    } else {
        return false;
    }
#else
    return PathFileExists(file.c_str());
/*
        // Works for normal drive letter without subdir ('D:') but fails for
   network drives without subdir ('\\localhost\D') HANDLE hFind;
    WIN32_FIND_DATAW FindData;

        hFind = FindFirstFileW(file.c_str(), &FindData);
        if(hFind == INVALID_HANDLE_VALUE)
                return false;
        else
        {
                FindClose(hFind);
                return true;
        }
*/
#endif
}

STRING remove_leading_curdir(STRING path) {
    if ((path.length() >= 2 && (path.substr(0, 2) == UNITXT(".\\"))) || path.substr(0, 2) == UNITXT("./")) {
        return path.substr(2, path.length() - 2);
    } else {
        return path;
    }
}

STRING remove_delimitor(STRING path) {
    size_t r = path.find_last_of(UNITXT("/\\"));
    if (r == path.length() - 1) {
        return path.substr(0, r);
    } else {
        return path;
    }
}

STRING remove_leading_delimitor(STRING path) {
    if (path.starts_with(UNITXT("\\")) || path.starts_with(UNITXT("/"))) {
        path.erase(path.begin());
    }
    return path;
}

bool ISNAMEDPIPE(int attributes) {
#ifdef WINDOWS
    (void)attributes;
    return false;
#else
    return (attributes & S_IFIFO) == S_IFIFO;
#endif
}

bool ISDIR(int attributes) {
#ifdef WINDOWS
    return ((FILE_ATTRIBUTE_DIRECTORY & attributes) != 0);
#else
    return S_ISDIR(attributes);
#endif
}

bool ISLINK(int attributes) {
#ifdef WINDOWS
    return ((FILE_ATTRIBUTE_REPARSE_POINT & attributes) != 0);
#else
    return S_ISLNK(attributes);
#endif
}

int get_attributes(STRING path, bool follow) {
#ifdef WINDOWS
    if (path.size() > 250) {
        path = wstring(L"\\\\?\\") + path;
    }

    (void)follow;
    if (path.length() == 2 && path.substr(1, 1) == UNITXT(":")) {
        path = path + UNITXT("\\");
    }

    DWORD attributes = GetFileAttributesW(path.c_str());

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        attributes = GetFileAttributesW(STRING(remove_delimitor(path)).c_str());
    }

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        attributes = GetFileAttributesW(STRING(remove_delimitor(path) + DELIM_STR).c_str());
    }

    if (INVALID_FILE_ATTRIBUTES == attributes) {
        return -1;
    }

    return attributes;

#else
    struct stat s;
    if (follow) {
        if (stat(path.c_str(), &s) < 0) {
            return -1;
        }
    } else {
        if (lstat(path.c_str(), &s) < 0) {
            return -1;
        }
    }

    return s.st_mode;

#endif
}

bool set_attributes(STRING path, int attributes) {
#ifdef WINDOWS
    attributes =
        attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);
    BOOL b = SetFileAttributesW(path.c_str(), attributes);
    return b;
#else
    return false;
#endif
}

bool is_dir(STRING path) { return ISDIR(get_attributes(path, false)); }

STRING str(uint64_t l) {
    CHR myBuff[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#ifdef WINDOWS
    SPRINTF(myBuff, UNITXT("%I64d"), l);
#else
    SPRINTF(myBuff, UNITXT("%llu"), static_cast<unsigned long long>(l));
#endif
    return ((STRING)myBuff);
}

void *tmalloc(size_t size) {
    void *p = malloc(size);

    abort(p == 0, UNITXT("Error at malloc() of %d MB. System out of memory."), (int)(size >> 20));
    return p;
}

#ifdef WINDOWS
int DeleteDirectory(const TCHAR *sPath) {
    HANDLE hFind; // file handle
    WIN32_FIND_DATA FindFileData;

    TCHAR DirPath[MAX_PATH_LEN];
    TCHAR FileName[MAX_PATH_LEN];

    wcscpy(DirPath, sPath);
    wcscat(DirPath, UNITXT("\\"));
    wcscpy(FileName, sPath);
    wcscat(FileName, UNITXT("\\*")); // searching all files

    hFind = FindFirstFile(FileName, &FindFileData); // find the first file
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (FindFileData.cFileName == STRING(UNITXT(".")) || FindFileData.cFileName == STRING(UNITXT(".."))) {
                continue;
            }

            wcscpy(FileName + STRLEN(DirPath), FindFileData.cFileName);

            if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                // we have found a directory, recurse
                if (!DeleteDirectory(FileName)) {
                    break; // directory couldn't be deleted
                }
            } else {
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                    _wchmod(FileName, _S_IWRITE); // change read-only file mode
                }

                if (!DeleteFile(FileName)) {
                    break; // file couldn't be deleted
                }
            }
        } while (FindNextFile(hFind, &FindFileData));

        FindClose(hFind); // closing file handle
    }

    return RemoveDirectoryW(sPath); // remove the empty (maybe not) directory
}
#endif

int delete_directory(STRING base_dir) {
#ifdef WINDOWS
    DeleteDirectory(base_dir.c_str());
#else

    STRING path;
    struct dirent *entry;
    DIR *dir;

    // process files
    if ((dir = opendir(base_dir.c_str())) != 0) {
        while ((entry = readdir(dir))) {
            if (STRING(entry->d_name) != UNITXT(".") && STRING(entry->d_name) != UNITXT("..")) {
                path = base_dir + DELIM_STR + STRING(entry->d_name);
                if (!is_dir(path)) {
                    REMOVE(path.c_str());
                }
            }
        }
        closedir(dir);
    }

    // process directories
    if ((dir = opendir(base_dir.c_str())) != 0) // todo, subst with findfirst because of unc paths like //?/ not
                                                // supported by stat()
    {
        while ((entry = readdir(dir))) {
            path = base_dir + DELIM_STR + STRING(entry->d_name);

            if (is_dir(path) && STRING(entry->d_name) != UNITXT(".") && STRING(entry->d_name) != UNITXT("..")) {
                delete_directory(path);
                rmdir(path.c_str());
            }
        }
        closedir(dir);
    }
    rmdir(base_dir.c_str());
#endif
    return 0;
}

vector<STRING> split_string(STRING str, STRING delim) {
    size_t cutAt;
    vector<STRING> results;
    while ((cutAt = str.find_first_of(delim)) != str.npos) {
        if (cutAt > 0) {
            results.push_back(str.substr(0, cutAt));
        }
        str = str.substr(cutAt + 1);
    }

    if (str.length() > 0) {
        results.push_back(str);
    }
    return results;
}

bool create_directory(STRING path) {
#ifdef WINDOWS
    return !CreateDirectoryW(path.c_str(), 0);
#else
    return mkdir(path.c_str(), 0777);
#endif
}

// todo fixme: this function is not implemented very generic
bool create_directories(STRING path) {
    return std::filesystem::create_directories(path);
}

bool equal2(const void *src1, const void *src2, size_t len) {
    char *s1 = (char *)src1;
    char *s2 = (char *)src2;

    for (unsigned int i = 0; i < len; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }
    return true;
}

bool same2(char *src, size_t len) {
    for (unsigned int i = 0; i < len; i++) {
        if (src[i] != src[0]) {
            return false;
        }
    }
    return true;
}

std::STRING del(int64_t l, size_t width) {
    CHR s[50], d[50];
    unsigned int i, j = 0;

    memset(s, 0, sizeof(s));
    memset(d, 0, sizeof(d));

    if (l == -1) {
        return std::STRING(CHR(' '), width);
    }

#ifdef WINDOWS
    SPRINTF(s, UNITXT("%I64d"), l);
#else
    SPRINTF(s, UNITXT("%llu"), static_cast<unsigned long long>(l));
#endif
    for (i = 0; i < STRLEN(s); i++) {
        if ((STRLEN(s) - i) % 3 == 0 && i != 0) {
            d[j] = ',';
            j++;
        }
        d[j] = s[i];
        j++;
    }

    std::STRING t = std::STRING(width > STRLEN(d) ? width - STRLEN(d) : 0, CHR(' '));
    t.append(std::STRING(d));
    return t;
}

size_t longest_common_prefix(vector<STRING> strings, bool case_sensitive) {
    if (strings.size() == 0) {
        return 0;
    }

    if (strings.size() == 1) {
        return strings[0].length();
    }

    size_t pos = 0;
    for (;;) {
        STRING c = UNITXT(""), d = UNITXT("");
        for (unsigned int i = 0; i < strings.size(); i++) {
            if (strings[i].length() < pos + 1) {
                return pos;
            }

            if (i == 0) {
                c = strings[i].substr(pos, 1);
            }

            d = strings[i].substr(pos, 1);

            if (case_sensitive && c.compare(d) != 0) {
                return pos;
            }

            if (!case_sensitive && lcase(c).compare(lcase(d)) != 0) {
                return pos;
            }
        }
        pos++;
    }
}

void set_bold(bool bold) {
#ifdef WINDOWS
    static WORD original = 7;
    WORD wColor;
    HANDLE hStdOut = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (bold) {
        if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
            original = csbi.wAttributes & 0x0F;
            if (original == 7) {
                wColor = (csbi.wAttributes & 0xF0) + (0xf & 0x0F);
                SetConsoleTextAttribute(hStdOut, wColor);
            }
        }
    } else if (!bold) {
        if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
            wColor = (csbi.wAttributes & 0xF0) + (original & 0x0F);
            SetConsoleTextAttribute(hStdOut, wColor);
        }
    }
#else
    if (bold) {
        FPRINTF(stderr, UNITXT("\033[1m"));
    } else {
        FPRINTF(stderr, UNITXT("\033[0m"));
    }

#endif
}

void tm_to_short(short_tm *s, tm *l) {
    s->tm_year = static_cast<short int>(l->tm_year);
    s->tm_sec = static_cast<unsigned char>(l->tm_sec);
    s->tm_min = static_cast<unsigned char>(l->tm_min);
    s->tm_hour = static_cast<unsigned char>(l->tm_hour);
    s->tm_mday = static_cast<unsigned char>(l->tm_mday);
    s->tm_mon = static_cast<unsigned char>(l->tm_mon);
    s->tm_wday = static_cast<unsigned char>(l->tm_wday);
    s->tm_yday = static_cast<unsigned char>(l->tm_yday);
    s->tm_isdst = static_cast<unsigned char>(l->tm_isdst);
}

void tm_to_long(short_tm *s, tm *l) {
    l->tm_sec = s->tm_sec;
    l->tm_min = s->tm_min;
    l->tm_hour = s->tm_hour;
    l->tm_mday = s->tm_mday;
    l->tm_mon = s->tm_mon;
    l->tm_year = s->tm_year;
    l->tm_wday = s->tm_wday;
    l->tm_yday = s->tm_yday;
    l->tm_isdst = s->tm_isdst;
}
