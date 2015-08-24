#ifndef MICROBIT_MANAGED_TYPE
#define MICROBIT_MANAGED_TYPE

/**
  * Class definition for a Generic Managed Type.
  *
  * Represents a reference counted object.
  *
  * @info When the destructor is called delete is called on the object - implicitly calling the given objects destructor.
  */
template <class T>
class ManagedType
{
private:

    int *ref;

public:

    T *object;

    /**
    * Constructor for the managed type, given a class space T.
    * @param object the object that you would like to be ref counted - of class T
    *
    * Example:
    * @code 
    * T object = new T();
    * ManagedType<T> mt(t);
    * @endcode
    */
    ManagedType(T* object);
    
  /**
    * Default constructor for the managed type, given a class space T.
    */
    ManagedType();

  /**
    * Copy constructor for the managed type, given a class space T.
    * @param t another managed type instance of class type T.
    *
    * Example:
    * @code 
    * T* object = new T();
    * ManagedType<T> mt(t);
    * ManagedType<T> mt1(mt);
    * @endcode
    */
    ManagedType(const ManagedType<T> &t);

  /**
    * Destructor for the managed type, given a class space T.
    */
    ~ManagedType();

  /**
    * Copy-assign member function for the managed type, given a class space.
    *
    * Example:
    * @code 
    * T* object = new T();
    * ManagedType<T> mt(t);
    * ManagedType<T> mt1 = mt;
    * @endcode
    */
    ManagedType<T>& operator = (const ManagedType<T>&i);
    
  /**
    * Returns the references to this ManagedType
    *
    * Example:
    * @code 
    * T* object = new T();
    * ManagedType<T> mt(t);
    * ManagedType<T> mt1(mt);
    *
    * mt.getReferences // this will be 2!
    * @endcode
    */
    int getReferences();

    T* operator->() {
        return object;
    }

    T* get() {
        return object;
    }
};

/**
* Constructor for the managed type, given a class space.
*/
template<typename T>
ManagedType<T>::ManagedType(T* object)
{
    this->object = object;
    ref = (int *)malloc(sizeof(int));
    *ref = 1;
}

/**
* Default constructor for the managed type, given a class space.
*/
template<typename T>
ManagedType<T>::ManagedType()
{
    this->object = NULL;
    ref = (int *)malloc(sizeof(int));
    *ref = 0;
}

/**
* Copy constructor for the managed type, given a class space.
*/
template<typename T>
ManagedType<T>::ManagedType(const ManagedType<T> &t)
{
    this->object = t.object;
    this->ref = t.ref;
    (*ref)++;
}

/**
* Destructor for the managed type, given a class space.
*/
template<typename T>
ManagedType<T>::~ManagedType()
{
    if (--(*ref) == 0)
    {
        delete object;
        free(ref);
    }
}

/**
* Copy-assign member function for the managed type, given a class space.
*/
template<typename T>
ManagedType<T>& ManagedType<T>::operator = (const ManagedType<T>&t)
{
    if (this == &t)
        return *this;

    if (--(*ref) == 0)
    {
        delete object;
        free(ref);
    }

    object = t.object;
    ref = t.ref;

    (*ref)++;

    return *this;
}

/**
* Returns the references to this ManagedType
*/
template<typename T>
int ManagedType<T>::getReferences()
{
    return (*ref);   
}
#endif
