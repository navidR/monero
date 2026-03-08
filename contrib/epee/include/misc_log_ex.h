// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


#ifndef _MISC_LOG_EX_H_
#define _MISC_LOG_EX_H_

#ifdef __cplusplus

#include <array>
#include <charconv>
#include <cstdio>
#include <ios>
#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>

#include "easylogging++.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "default"

#define MAX_LOG_FILE_SIZE 104850000 // 100 MB - 7600 bytes
#define MAX_LOG_FILES 10

namespace epee
{
namespace logging
{
  namespace detail
  {
    struct format_state
    {
      std::string output;
      int base = 10;
      bool boolalpha = false;
    };

    inline void append_escaped_literal(std::string& out, std::string_view literal)
    {
      std::size_t pos = 0;
      while (pos < literal.size())
      {
        const std::size_t brace = literal.find_first_of("{}", pos);
        if (brace == std::string_view::npos)
        {
          out.append(literal.data() + pos, literal.size() - pos);
          return;
        }

        out.append(literal.data() + pos, brace - pos);
        if (brace + 1 < literal.size() && literal[brace + 1] == literal[brace])
        {
          out.push_back(literal[brace]);
          pos = brace + 2;
        }
        else
        {
          out.push_back(literal[brace]);
          pos = brace + 1;
        }
      }
    }
  }

  class string_streambuf : public std::streambuf
  {
  public:
    string_streambuf()
    {
      buffer_.reserve(256);
    }

    const std::string& str() const noexcept
    {
      return buffer_;
    }

  protected:
    std::streamsize xsputn(const char_type* s, std::streamsize count) override
    {
      if (count > 0)
        buffer_.append(s, static_cast<std::size_t>(count));
      return count;
    }

    int_type overflow(int_type ch) override
    {
      if (traits_type::eq_int_type(ch, traits_type::eof()))
        return traits_type::not_eof(ch);

      buffer_.push_back(traits_type::to_char_type(ch));
      return ch;
    }

    int sync() override
    {
      return 0;
    }

  private:
    std::string buffer_;
  };

  class string_ostream : public std::ostream
  {
  public:
    string_ostream()
      : std::ostream(&buffer_)
    {
    }

    const std::string& str() const noexcept
    {
      return buffer_.str();
    }

  private:
    string_streambuf buffer_;
  };

  template <typename T>
  std::string stream_value(const T& value)
  {
    string_ostream out;
    out << value;
    return out.str();
  }

  inline void append_string(detail::format_state& state, std::string_view value)
  {
    state.output.append(value.data(), value.size());
  }

  inline void append_string(detail::format_state& state, const char* value)
  {
    if (value)
      state.output += value;
    else
      state.output += "(null)";
  }

  inline void append_string(detail::format_state& state, char* value)
  {
    append_string(state, static_cast<const char*>(value));
  }

  inline void append_char(detail::format_state& state, char value)
  {
    state.output.push_back(value);
  }

  template <typename T>
  std::enable_if_t<std::is_enum_v<T>, void> append_value(detail::format_state& state, const T& value);

  template <typename T>
  std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> &&
    !std::is_same_v<T, char> && !std::is_same_v<T, signed char> && !std::is_same_v<T, unsigned char>, void>
  append_integer(detail::format_state& state, T value)
  {
    std::array<char, 96> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, state.base);
    if (result.ec == std::errc())
    {
      state.output.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
      return;
    }

    state.output += stream_value(value);
  }

  inline void append_value(detail::format_state& state, bool value)
  {
    if (state.boolalpha)
      state.output += value ? "true" : "false";
    else
      state.output.push_back(value ? '1' : '0');
  }

  inline void append_value(detail::format_state& state, char value)
  {
    append_char(state, value);
  }

  inline void append_value(detail::format_state& state, signed char value)
  {
    append_char(state, static_cast<char>(value));
  }

  inline void append_value(detail::format_state& state, unsigned char value)
  {
    append_char(state, static_cast<char>(value));
  }

  inline void append_value(detail::format_state& state, const std::string& value)
  {
    append_string(state, value);
  }

  inline void append_value(detail::format_state& state, std::string_view value)
  {
    append_string(state, value);
  }

  inline void append_value(detail::format_state& state, const char* value)
  {
    append_string(state, value);
  }

  inline void append_value(detail::format_state& state, char* value)
  {
    append_string(state, value);
  }

  template <typename T>
  std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> &&
    !std::is_same_v<T, char> && !std::is_same_v<T, signed char> && !std::is_same_v<T, unsigned char>, void>
  append_value(detail::format_state& state, T value)
  {
    append_integer(state, value);
  }

  template <typename T>
  std::enable_if_t<std::is_floating_point_v<T>, void> append_value(detail::format_state& state, T value)
  {
    std::array<char, 128> buffer{};
    const int written = std::snprintf(buffer.data(), buffer.size(), "%g", static_cast<double>(value));
    if (written > 0 && static_cast<std::size_t>(written) < buffer.size())
    {
      state.output.append(buffer.data(), static_cast<std::size_t>(written));
      return;
    }

    state.output += stream_value(value);
  }

  inline void append_value(detail::format_state& state, std::ios_base& (*manip)(std::ios_base&))
  {
    if (manip == static_cast<std::ios_base& (*)(std::ios_base&)>(std::boolalpha))
      state.boolalpha = true;
    else if (manip == static_cast<std::ios_base& (*)(std::ios_base&)>(std::noboolalpha))
      state.boolalpha = false;
    else if (manip == static_cast<std::ios_base& (*)(std::ios_base&)>(std::hex))
      state.base = 16;
    else if (manip == static_cast<std::ios_base& (*)(std::ios_base&)>(std::dec))
      state.base = 10;
    else if (manip == static_cast<std::ios_base& (*)(std::ios_base&)>(std::oct))
      state.base = 8;
    else
      state.output += stream_value(manip);
  }

  inline void append_value(detail::format_state& state, std::ostream& (*manip)(std::ostream&))
  {
    if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl<char, std::char_traits<char>>))
      state.output.push_back('\n');
    else
      state.output += stream_value(manip);
  }

  template <typename T>
  std::enable_if_t<std::is_enum_v<T>, void> append_value(detail::format_state& state, const T& value)
  {
    append_integer(state, static_cast<std::underlying_type_t<T>>(value));
  }

  template <typename T>
  std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_enum_v<T> &&
    !std::is_same_v<std::decay_t<T>, std::string> && !std::is_same_v<std::decay_t<T>, std::string_view> &&
    !std::is_same_v<std::decay_t<T>, const char*> && !std::is_same_v<std::decay_t<T>, char*> &&
    !std::is_same_v<std::decay_t<T>, std::ios_base& (*)(std::ios_base&)> &&
    !std::is_same_v<std::decay_t<T>, std::ostream& (*)(std::ostream&)>, void>
  append_value(detail::format_state& state, const T& value)
  {
    state.output += stream_value(value);
  }

  inline void append_format_tail(detail::format_state& state, std::string_view format)
  {
    detail::append_escaped_literal(state.output, format);
  }

  inline void append_format(detail::format_state& state, std::string_view format)
  {
    append_format_tail(state, format);
  }

  template <typename T, typename... Args>
  void append_format(detail::format_state& state, std::string_view format, T&& value, Args&&... args)
  {
    std::size_t pos = 0;
    while (pos < format.size())
    {
      const std::size_t brace = format.find_first_of("{}", pos);
      if (brace == std::string_view::npos)
      {
        detail::append_escaped_literal(state.output, format.substr(pos));
        append_value(state, std::forward<T>(value));
        (append_value(state, std::forward<Args>(args)), ...);
        return;
      }

      if (brace > pos)
        state.output.append(format.data() + pos, brace - pos);

      if (brace + 1 < format.size() && format[brace + 1] == format[brace])
      {
        state.output.push_back(format[brace]);
        pos = brace + 2;
        continue;
      }

      if (format[brace] == '{' && brace + 1 < format.size() && format[brace + 1] == '}')
      {
        append_value(state, std::forward<T>(value));
        append_format(state, format.substr(brace + 2), std::forward<Args>(args)...);
        return;
      }

      state.output.push_back(format[brace]);
      pos = brace + 1;
    }

    append_value(state, std::forward<T>(value));
    (append_value(state, std::forward<Args>(args)), ...);
  }

  inline std::string format_message(std::string_view format)
  {
    return std::string(format);
  }

  template <typename... Args>
  std::string format_message(std::string_view format, Args&&... args)
  {
    detail::format_state state;
    state.output.reserve(format.size() + sizeof...(Args) * 8);
    append_format(state, format, std::forward<Args>(args)...);
    return state.output;
  }

  template <typename... Args>
  void dispatch_formatted(el::Level level, const char* category, el::Color color,
    const char* file, int line, const char* func, el::base::DispatchAction type,
    std::string_view format, Args&&... args)
  {
    const std::string message = format_message(format, std::forward<Args>(args)...);
    el::base::Writer(level, color, file, line, func, type).construct(category) << message;
  }

  template <typename T>
  void dispatch_value(el::Level level, const char* category, el::Color color,
    const char* file, int line, const char* func, el::base::DispatchAction type,
    T&& value)
  {
    detail::format_state state;
    append_value(state, std::forward<T>(value));
    el::base::Writer(level, color, file, line, func, type).construct(category) << state.output;
  }

  template <typename Prefix, typename... Args>
  void dispatch_prefixed(el::Level level, const char* category, el::Color color,
    const char* file, int line, const char* func, el::base::DispatchAction type,
    Prefix&& prefix, std::string_view format, Args&&... args)
  {
    detail::format_state state;
    state.output.reserve(format.size() + sizeof...(Args) * 8 + 32);
    append_value(state, std::forward<Prefix>(prefix));
    append_format(state, format, std::forward<Args>(args)...);
    el::base::Writer(level, color, file, line, func, type).construct(category) << state.output;
  }

  template <typename T>
  std::string to_message_string(T&& value)
  {
    detail::format_state state;
    append_value(state, std::forward<T>(value));
    return std::move(state.output);
  }
}
}

#define MLOG_PP_CAT_(a, b) a##b
#define MLOG_PP_CAT(a, b) MLOG_PP_CAT_(a, b)
#define MLOG_PP_ARG_N( \
   _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
  _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
  _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
  _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
  _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
  _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
  _61, _62, _63, _64, _65, N, ...) N
#define MLOG_PP_RSEQ_N() \
  65, 64, 63, 62, 61, 60, 59, 58, 57, 56, \
  55, 54, 53, 52, 51, 50, 49, 48, 47, 46, \
  45, 44, 43, 42, 41, 40, 39, 38, 37, 36, \
  35, 34, 33, 32, 31, 30, 29, 28, 27, 26, \
  25, 24, 23, 22, 21, 20, 19, 18, 17, 16, \
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, \
  5, 4, 3, 2, 1, 0
#define MLOG_PP_NARG_(...) MLOG_PP_ARG_N(__VA_ARGS__)
#define MLOG_PP_NARG(...) MLOG_PP_NARG_(__VA_ARGS__, MLOG_PP_RSEQ_N())

#define MCLOG_FORMAT(level, cat, color, type, fmt, ...) do { \
    if (el::Loggers::allowed(level, cat)) { \
      epee::logging::dispatch_formatted(level, cat, color, __FILE__, __LINE__, ELPP_FUNC, type, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

#define MCLOG_VALUE(level, cat, color, type, value) do { \
    if (el::Loggers::allowed(level, cat)) { \
      epee::logging::dispatch_value(level, cat, color, __FILE__, __LINE__, ELPP_FUNC, type, value); \
    } \
  } while (0)

#define MCLOG_TYPE_1(level, cat, color, type, x) MCLOG_VALUE(level, cat, color, type, x)
#define MCLOG_TYPE_2(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_3(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_4(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_5(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_6(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_7(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_8(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_9(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_10(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_11(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_12(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_13(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_14(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_15(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_16(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_17(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_18(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_19(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_20(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_21(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_22(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_23(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_24(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_25(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_26(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_27(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_28(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_29(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_30(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_31(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_32(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_33(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_34(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_35(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_36(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_37(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_38(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_39(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_40(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_41(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_42(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_43(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_44(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_45(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_46(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_47(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_48(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_49(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_50(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_51(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_52(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_53(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_54(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_55(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_56(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_57(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_58(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_59(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_60(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_61(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_62(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_63(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)
#define MCLOG_TYPE_64(level, cat, color, type, fmt, ...) MCLOG_FORMAT(level, cat, color, type, fmt, ##__VA_ARGS__)

#define MCLOG_TYPE(level, cat, color, type, ...) \
  MLOG_PP_CAT(MCLOG_TYPE_, MLOG_PP_NARG(__VA_ARGS__))(level, cat, color, type, __VA_ARGS__)

#define MCLOG(level, cat, color, ...) MCLOG_TYPE(level, cat, color, el::base::DispatchAction::NormalLog, __VA_ARGS__)
#define MCLOG_FILE(level, cat, ...) MCLOG_TYPE(level, cat, el::Color::Default, el::base::DispatchAction::FileOnlyLog, __VA_ARGS__)

#define MCFATAL(cat,...) MCLOG(el::Level::Fatal,cat, el::Color::Default, __VA_ARGS__)
#define MCERROR(cat,...) MCLOG(el::Level::Error,cat, el::Color::Default, __VA_ARGS__)
#define MCWARNING(cat,...) MCLOG(el::Level::Warning,cat, el::Color::Default, __VA_ARGS__)
#define MCINFO(cat,...) MCLOG(el::Level::Info,cat, el::Color::Default, __VA_ARGS__)
#define MCDEBUG(cat,...) MCLOG(el::Level::Debug,cat, el::Color::Default, __VA_ARGS__)
#define MCTRACE(cat,...) MCLOG(el::Level::Trace,cat, el::Color::Default, __VA_ARGS__)

#define MCLOG_COLOR(level,cat,color,...) MCLOG(level,cat,color,__VA_ARGS__)
#define MCLOG_RED(level,cat,...) MCLOG_COLOR(level,cat,el::Color::Red,__VA_ARGS__)
#define MCLOG_GREEN(level,cat,...) MCLOG_COLOR(level,cat,el::Color::Green,__VA_ARGS__)
#define MCLOG_YELLOW(level,cat,...) MCLOG_COLOR(level,cat,el::Color::Yellow,__VA_ARGS__)
#define MCLOG_BLUE(level,cat,...) MCLOG_COLOR(level,cat,el::Color::Blue,__VA_ARGS__)
#define MCLOG_MAGENTA(level,cat,...) MCLOG_COLOR(level,cat,el::Color::Magenta,__VA_ARGS__)
#define MCLOG_CYAN(level,cat,...) MCLOG_COLOR(level,cat,el::Color::Cyan,__VA_ARGS__)

#define MLOG_RED(level,...) MCLOG_RED(level,MONERO_DEFAULT_LOG_CATEGORY,__VA_ARGS__)
#define MLOG_GREEN(level,...) MCLOG_GREEN(level,MONERO_DEFAULT_LOG_CATEGORY,__VA_ARGS__)
#define MLOG_YELLOW(level,...) MCLOG_YELLOW(level,MONERO_DEFAULT_LOG_CATEGORY,__VA_ARGS__)
#define MLOG_BLUE(level,...) MCLOG_BLUE(level,MONERO_DEFAULT_LOG_CATEGORY,__VA_ARGS__)
#define MLOG_MAGENTA(level,...) MCLOG_MAGENTA(level,MONERO_DEFAULT_LOG_CATEGORY,__VA_ARGS__)
#define MLOG_CYAN(level,...) MCLOG_CYAN(level,MONERO_DEFAULT_LOG_CATEGORY,__VA_ARGS__)

#define MFATAL(...) MCFATAL(MONERO_DEFAULT_LOG_CATEGORY, __VA_ARGS__)
#define MERROR(...) MCERROR(MONERO_DEFAULT_LOG_CATEGORY, __VA_ARGS__)
#define MWARNING(...) MCWARNING(MONERO_DEFAULT_LOG_CATEGORY, __VA_ARGS__)
#define MINFO(...) MCINFO(MONERO_DEFAULT_LOG_CATEGORY, __VA_ARGS__)
#define MDEBUG(...) MCDEBUG(MONERO_DEFAULT_LOG_CATEGORY, __VA_ARGS__)
#define MTRACE(...) MCTRACE(MONERO_DEFAULT_LOG_CATEGORY, __VA_ARGS__)
#define MLOG(level,...) MCLOG(level,MONERO_DEFAULT_LOG_CATEGORY,el::Color::Default, __VA_ARGS__)

#define MGINFO(...) MCINFO("global", __VA_ARGS__)
#define MGINFO_RED(...) MCLOG_RED(el::Level::Info, "global", __VA_ARGS__)
#define MGINFO_GREEN(...) MCLOG_GREEN(el::Level::Info, "global", __VA_ARGS__)
#define MGINFO_YELLOW(...) MCLOG_YELLOW(el::Level::Info, "global", __VA_ARGS__)
#define MGINFO_BLUE(...) MCLOG_BLUE(el::Level::Info, "global", __VA_ARGS__)
#define MGINFO_MAGENTA(...) MCLOG_MAGENTA(el::Level::Info, "global", __VA_ARGS__)
#define MGINFO_CYAN(...) MCLOG_CYAN(el::Level::Info, "global", __VA_ARGS__)

#define IFLOG(level, cat, color, type, init, fmt, ...) \
  do { \
    if (el::Loggers::allowed(level, cat)) { \
      init; \
      epee::logging::dispatch_formatted(level, cat, color, __FILE__, __LINE__, ELPP_FUNC, type, fmt, ##__VA_ARGS__); \
    } \
  } while(0)
#define MIDEBUG(init, ...) IFLOG(el::Level::Debug, MONERO_DEFAULT_LOG_CATEGORY, el::Color::Default, el::base::DispatchAction::NormalLog, init, __VA_ARGS__)


#define LOG_ERROR(...) MERROR(__VA_ARGS__)
#define LOG_PRINT_L0(...) MWARNING(__VA_ARGS__)
#define LOG_PRINT_L1(...) MINFO(__VA_ARGS__)
#define LOG_PRINT_L2(...) MDEBUG(__VA_ARGS__)
#define LOG_PRINT_L3(...) MTRACE(__VA_ARGS__)
#define LOG_PRINT_L4(...) MTRACE(__VA_ARGS__)

#define _dbg3(...) MTRACE(__VA_ARGS__)
#define _dbg2(...) MDEBUG(__VA_ARGS__)
#define _dbg1(...) MDEBUG(__VA_ARGS__)
#define _info(...) MINFO(__VA_ARGS__)
#define _note(...) MDEBUG(__VA_ARGS__)
#define _fact(...) MDEBUG(__VA_ARGS__)
#define _mark(...) MDEBUG(__VA_ARGS__)
#define _warn(...) MWARNING(__VA_ARGS__)
#define _erro(...) MERROR(__VA_ARGS__)

#define MLOG_SET_THREAD_NAME(x) el::Helpers::setThreadName(x)

#ifndef LOCAL_ASSERT
#include <assert.h>
#if (defined _MSC_VER)
#define LOCAL_ASSERT(expr) {if(epee::debug::get_set_enable_assert()){_ASSERTE(expr);}}
#else
#define LOCAL_ASSERT(expr)
#endif

#endif

std::string mlog_get_default_log_path(const char *default_filename);
void mlog_configure(const std::string &filename_base, bool console, const std::size_t max_log_file_size = MAX_LOG_FILE_SIZE, const std::size_t max_log_files = MAX_LOG_FILES);
void mlog_set_categories(const char *categories);
std::string mlog_get_categories();
void mlog_set_log_level(int level);
void mlog_set_log(const char *log);

namespace epee
{
namespace debug
{
  inline bool get_set_enable_assert(bool set = false, bool v = false)
  {
    static bool e = true;
    if(set)
      e = v;
    return e;
  }
}



#define ENDL static_cast<std::ostream& (*)(std::ostream&)>(std::endl<char, std::char_traits<char>>)

#define TRY_ENTRY()   try {
#define CATCH_ENTRY(location, return_val) } \
  catch(const std::exception& ex) \
{ \
  (void)(ex); \
  LOG_ERROR("Exception at [{}], what={}", location, ex.what()); \
  return return_val; \
}\
  catch(...)\
{\
  LOG_ERROR("Exception at [{}], generic exception \"...\"", location);\
  return return_val; \
}

#define CATCH_ENTRY_L0(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L1(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L2(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L3(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L4(lacation, return_val) CATCH_ENTRY(lacation, return_val)


#define ASSERT_MES_AND_THROW(...) do { \
  const std::string monero_message = epee::logging::format_message(__VA_ARGS__); \
  LOG_ERROR("{}", monero_message); \
  throw std::runtime_error(monero_message); \
} while (0)
#define CHECK_AND_ASSERT_THROW_MES(expr, ...) do {if(!(expr)) ASSERT_MES_AND_THROW(__VA_ARGS__);} while(0)


#ifndef CHECK_AND_ASSERT
#define CHECK_AND_ASSERT(expr, fail_ret_val)   do{if(!(expr)){LOCAL_ASSERT(expr); return fail_ret_val;};}while(0)
#endif

#ifndef CHECK_AND_ASSERT_MES
#define CHECK_AND_ASSERT_MES(expr, fail_ret_val, ...) do { \
  if (!(expr)) { \
    LOG_ERROR("{}", epee::logging::format_message(__VA_ARGS__)); \
    return fail_ret_val; \
  } \
} while (0)
#endif

#ifndef CHECK_AND_NO_ASSERT_MES_L
#define CHECK_AND_NO_ASSERT_MES_L(expr, fail_ret_val, l, ...) do { \
  if (!(expr)) { \
    LOG_PRINT_L##l("{}", epee::logging::format_message(__VA_ARGS__)); \
    return fail_ret_val; \
  } \
} while (0)
#endif

#ifndef CHECK_AND_NO_ASSERT_MES
#define CHECK_AND_NO_ASSERT_MES(expr, fail_ret_val, ...) CHECK_AND_NO_ASSERT_MES_L(expr, fail_ret_val, 0, __VA_ARGS__)
#endif

#ifndef CHECK_AND_NO_ASSERT_MES_L1
#define CHECK_AND_NO_ASSERT_MES_L1(expr, fail_ret_val, ...) CHECK_AND_NO_ASSERT_MES_L(expr, fail_ret_val, 1, __VA_ARGS__)
#endif


#ifndef CHECK_AND_ASSERT_MES_NO_RET
#define CHECK_AND_ASSERT_MES_NO_RET(expr, ...) do { \
  if (!(expr)) { \
    LOG_ERROR("{}", epee::logging::format_message(__VA_ARGS__)); \
    return; \
  } \
} while (0)
#endif


#ifndef CHECK_AND_ASSERT_MES2
#define CHECK_AND_ASSERT_MES2(expr, ...) do { \
  if (!(expr)) { \
    LOG_ERROR("{}", epee::logging::format_message(__VA_ARGS__)); \
  } \
} while (0)
#endif

enum console_colors
{
  console_color_default,
  console_color_white,
  console_color_red,
  console_color_green,
  console_color_blue,
  console_color_cyan,
  console_color_magenta,
  console_color_yellow
};

bool is_stdout_a_tty();
void set_console_color(int color, bool bright);
void reset_console_color();

}

extern "C"
{

#endif

#ifdef __GNUC__
#define ATTRIBUTE_PRINTF __attribute__((format(printf, 2, 3)))
#else
#define ATTRIBUTE_PRINTF
#endif

bool merror(const char *category, const char *format, ...) ATTRIBUTE_PRINTF;
bool mwarning(const char *category, const char *format, ...) ATTRIBUTE_PRINTF;
bool minfo(const char *category, const char *format, ...) ATTRIBUTE_PRINTF;
bool mdebug(const char *category, const char *format, ...) ATTRIBUTE_PRINTF;
bool mtrace(const char *category, const char *format, ...) ATTRIBUTE_PRINTF;

#ifdef __cplusplus

}

#endif

#endif //_MISC_LOG_EX_H_
