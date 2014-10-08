/** A simple concrete-type delegate. 

    Modified version of:
    http://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates
    
    Original author: Sergey Ryazanov
    
    @todo Templatize, or use his (or someone elses) full version
 */
class delegate
{
public:
  delegate()
    : object_ptr(0)
    , stub_ptr(0)
  {}
  
  template <class T, void (T::*TMethod)()>
  static delegate from_method(T* object_ptr)
  {
    delegate d;
    d.object_ptr = object_ptr;
    d.stub_ptr = &method_stub<T, TMethod>;
    return d;
  }

  void operator()() const
  {
    return (*stub_ptr)(object_ptr);
  }

private:
  typedef void (*stub_type)(void* object_ptr);

  void* object_ptr;
  stub_type stub_ptr;

  template <class T, void (T::*TMethod)()>
  static void method_stub(void* object_ptr)
  {
    T* p = static_cast<T*>(object_ptr);
    return (p->*TMethod)();
  }
};

