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
