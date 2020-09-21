#pragma once

#include "priv/simple_signal.hpp"

namespace evnt
{
template <typename SignalSignature,
          class Collector = Simple::Lib::CollectorDefault<typename std::function<SignalSignature>::result_type> >
using signal = Simple::Signal<SignalSignature, Collector>;

template <class Instance, class Class, class R, class... Args> std::function<R (Args...)>
inline slot (Instance &object, R (Class::*method) (Args...)) { return Simple::slot(object, method); }

template<class Class, class R, class... Args> std::function<R (Args...)>
slot (Class *object, R (Class::*method) (Args...)) { return Simple::slot(object, method); }

template <typename Result>
using Collector_until_0 = Simple::CollectorUntil0<Result>;

template <typename Result>
using Collector_while_0 = Simple::CollectorWhile0<Result>;

template<typename Result>
using Collector_vector = Simple::CollectorVector<Result>;
}
