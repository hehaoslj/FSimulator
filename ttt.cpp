fprintf:
printf
write

%s --> const char*

fwprintf
wsprintf

%s --> const wchar_t*


std::string --> std::base_string<char>

s.c_str() --> const char*

std::wstring --> std::base_string<wchar_t>

ws.c_str() --> const wchar_t*


wchar_t* --> char*

WideCharToMultiByte

char* --> wchar_t*

L"1234" -->1234 wtoi
"1234" --> 1234 atoi
MultiByteToWideChar

CreateWindow --> CreateWindowA CreateWindowW

windows: widechar --> utf16(LE)  wchar_t [] *, std::wstring
                                               wcout

ANSI ASC-II codeset

Unicode codeset

字节序
1234 3412

UTF-8 无字节序

总结
程序运行时 runtime wchar_t wstring  --> utf16
程序外部 文件系统 数据库 char string --> utf8
