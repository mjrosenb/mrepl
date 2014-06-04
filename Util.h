#if 0
template <typename T>
T prev(T &x)
{
    T tmp = x;
    return --tmp;
}
#endif
list<Line*>::iterator prev(list<Line*>::iterator &x)
{
    list<Line*>::iterator tmp = x;
    return --tmp;
}
template <typename T>
typename list<T>::iterator next(typename list<T>::iterator &x)
{

    typename list<T>::iterator tmp = x;
    return ++tmp;
}

