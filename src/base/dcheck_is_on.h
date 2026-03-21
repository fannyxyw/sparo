// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_DCHECK_IS_ON_H_
#define BASE_DCHECK_IS_ON_H_

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() false
#else
#define DCHECK_IS_ON() true
#endif

#if 0
#define EXPENSIVE_DCHECKS_ARE_ON() true
#else
#define EXPENSIVE_DCHECKS_ARE_ON() false
#endif

#endif  // BASE_DCHECK_IS_ON_H_
