/* Copyright (c) 2017, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdio.h>
#include <vector>

#include <gtest/gtest.h>

#include <openssl/ssl.h>

BSSL_NAMESPACE_BEGIN
namespace {

static void TestCtor(Span<int> s, const int *ptr, size_t size) {
  EXPECT_EQ(s.data(), ptr);
  EXPECT_EQ(s.size(), size);
}

static void TestConstCtor(Span<const int> s, const int *ptr, size_t size) {
  EXPECT_EQ(s.data(), ptr);
  EXPECT_EQ(s.size(), size);
}

TEST(SpanTest, CtorEmpty) {
  Span<int> s;
  TestCtor(s, nullptr, 0);
}

TEST(SpanTest, CtorFromPtrAndSize) {
  std::vector<int> v = {7, 8, 9, 10};
  Span<int> s(v.data(), v.size());
  TestCtor(s, v.data(), v.size());
}

TEST(SpanTest, CtorFromVector) {
  std::vector<int> v = {1, 2};
  // Const ctor is implicit.
  TestConstCtor(v, v.data(), v.size());
  // Mutable is explicit.
  Span<int> s(v);
  TestCtor(s, v.data(), v.size());
}

TEST(SpanTest, CtorConstFromArray) {
  int v[] = {10, 11};
  // Array ctor is implicit for const and mutable T.
  TestConstCtor(v, v, 2);
  TestCtor(v, v, 2);
}

TEST(SpanTest, MakeSpan) {
  std::vector<int> v = {100, 200, 300};
  TestCtor(MakeSpan(v), v.data(), v.size());
  TestCtor(MakeSpan(v.data(), v.size()), v.data(), v.size());
  TestConstCtor(MakeSpan(v.data(), v.size()), v.data(), v.size());
  TestConstCtor(MakeSpan(v), v.data(), v.size());
}

TEST(SpanTest, MakeConstSpan) {
  std::vector<int> v = {100, 200, 300};
  TestConstCtor(MakeConstSpan(v), v.data(), v.size());
  TestConstCtor(MakeConstSpan(v.data(), v.size()), v.data(), v.size());
  // But not:
  // TestConstCtor(MakeSpan(v), v.data(), v.size());
}

TEST(SpanTest, Accessor) {
  std::vector<int> v({42, 23, 5, 101, 80});
  Span<int> s(v);
  for (size_t i = 0; i < s.size(); ++i) {
    EXPECT_EQ(s[i], v[i]);
    EXPECT_EQ(s.at(i), v.at(i));
  }
  EXPECT_EQ(s.begin(), v.data());
  EXPECT_EQ(s.end(), v.data() + v.size());
}

TEST(SpanTest, ConstExpr) {
  static constexpr int v[] = {1, 2, 3, 4};
  constexpr bssl::Span<const int> span1(v);
  static_assert(span1.size() == 4u, "wrong size");
  constexpr bssl::Span<const int> span2 = MakeConstSpan(v);
  static_assert(span2.size() == 4u, "wrong size");
  static_assert(span2.subspan(1).size() == 3u, "wrong size");
  static_assert(span2.first(1).size() == 1u, "wrong size");
  static_assert(span2.last(1).size() == 1u, "wrong size");
  static_assert(span2[0] == 1, "wrong value");
}

TEST(SpanDeathTest, BoundsChecks) {
  // Make an array that's larger than we need, so that a failure to bounds check
  // won't crash.
  const int v[] = {1, 2, 3, 4};
  Span<const int> span(v, 3);
  // Out of bounds access.
  EXPECT_DEATH_IF_SUPPORTED(span[3], "");
  EXPECT_DEATH_IF_SUPPORTED(span.subspan(4), "");
  EXPECT_DEATH_IF_SUPPORTED(span.first(4), "");
  EXPECT_DEATH_IF_SUPPORTED(span.last(4), "");
  // Accessing an empty span.
  Span<const int> empty(v, 0);
  EXPECT_DEATH_IF_SUPPORTED(empty[0], "");
  EXPECT_DEATH_IF_SUPPORTED(empty.front(), "");
  EXPECT_DEATH_IF_SUPPORTED(empty.back(), "");
}

}  // namespace
BSSL_NAMESPACE_END