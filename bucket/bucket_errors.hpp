// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BUCKET_BUCKET_ERRORS_HPP
#define BUCKET_BUCKET_ERRORS_HPP

namespace bucket {

class BucketException : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class ObjectNotFound : public BucketException {
public:
  using BucketException::BucketException;
};

class CannotCreateObject : public BucketException {
public:
  using BucketException::BucketException;
};

class ConstraintException : public BucketException {
public:
  using BucketException::BucketException;
};

class ConstraintUnique : public ConstraintException {
public:
  using ConstraintException::ConstraintException;
};

class ConstraintNull : public ConstraintException {
public:
  using ConstraintException::ConstraintException;
};

} //< namespace bucket

#endif //< BUCKET_BUCKET_ERRORS_HPP
