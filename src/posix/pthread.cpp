// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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


int nanosleep(const struct timespec*rqtp, struct timespec *rmtp) {
  return 0;
};


int pthread_cond_broadcast(pthread_cond_t *cond) {
  return 0;
};

int pthread_cond_signal(pthread_cond_t *cond) {
  return 0;
};

int pthread_cond_timedwait(pthread_cond_t *restrict cond,
                           pthread_mutex_t *restrict mutex,
                           const struct timespec *restrict abstime) {
  return 0;
};


int pthread_cond_wait(pthread_cond_t *restrict cond,
                      pthread_mutex_t *restrict mutex) {
  return 0;
}


int pthread_detach(pthread_t thread) {
  return 0;
};


int pthread_equal(pthread_t t1, pthread_t t2) {
return 1;
};


void *pthread_getspecific(pthread_key_t key) {

};

int pthread_setspecific(pthread_key_t key, const void *value) {
return 1;
};


int pthread_join(pthread_t thread, void **value_ptr) {

};

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *restrict mutex,
                         const pthread_mutexattr_t *restrict attr);
