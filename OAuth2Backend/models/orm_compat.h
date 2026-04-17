#pragma once

// C++17/20 compatibility for std::codecvt_utf8_utf16
// This header provides a fallback implementation when std::codecvt_utf8_utf16 is not available

#include <locale>
#include <codecvt>
#include <algorithm>
#include <string>
#include <cwchar>

// Check if codecvt_utf8_utf16 is available
#if __cplusplus >= 202002L || (defined(_MSC_VER) && _MSVC_LANG >= 202002L)
    // C++20 or later: codecvt_utf8_utf16 is removed, provide fallback
    #include <system_error>
    #include <vector>

    // Fallback implementation for C++20
    namespace std {
        template<typename Elem, unsigned long Maxcode = 0x10ffff, std::codecvt_mode Mode = (std::codecvt_mode)0>
        class codecvt_utf8_utf16 : public std::codecvt<Elem, char, std::mbstate_t> {
        public:
            explicit codecvt_utf8_utf16(size_t refs = 0) : std::codecvt<Elem, char, std::mbstate_t>(refs) {}

        protected:
            typename std::codecvt<Elem, char, std::mbstate_t>::result
            do_out(std::mbstate_t& state, const Elem* from, const Elem* from_end, const Elem*& from_next,
                   char* to, char* to_end, char*& to_next) const override {
                // Simple UTF-16 to UTF-8 conversion
                while (from < from_end && to < to_end - 3) {
                    wchar_t wc = *from;
                    if (wc < 0x80) {
                        *to++ = static_cast<char>(wc);
                    } else if (wc < 0x800) {
                        *to++ = static_cast<char>(0xC0 | (wc >> 6));
                        *to++ = static_cast<char>(0x80 | (wc & 0x3F));
                    } else {
                        *to++ = static_cast<char>(0xE0 | (wc >> 12));
                        *to++ = static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                        *to++ = static_cast<char>(0x80 | (wc & 0x3F));
                    }
                    from++;
                }
                from_next = from;
                to_next = to;
                return std::codecvt<Elem, char, std::mbstate_t>::ok;
            }
        };
    }
#endif
