#pragma once
#include <stdint.h>
#include <stddef.h>
#include "base/memory_pool.h"

namespace Astra {

class FuncBindTypeHelper {
protected:
	template<bool IS_POINTER, typename Type>
	struct ToPtr {
		static inline Type Get(Type& t) {
			return t;
		}
	};

	template<typename Type>
	struct ToPtr<false, Type> {
		static inline Type* Get(Type& t) {
			return &t;
		}
	};

public:

	template<typename Type>
	struct IsPtr {
		static const bool value = std::is_pointer<typename std::remove_reference<Type>::type>::value;
	};

	template<typename Type>
	struct PtrType {
		typedef typename std::conditional<FuncBindTypeHelper::IsPtr<Type>::value
			, typename std::remove_reference<Type>::type
			, typename std::add_pointer<Type>::type
		>::type type;
	};

	template<typename Type>
	static inline typename PtrType<Type>::type ToPtr(Type& data) {
		return ToPtr<FuncBindTypeHelper::IsPtr<Type>::value, Type>::Get(data);
	}


	template<typename ObjType, typename FuncType, typename... Args>
	struct ObjFuncRetType {
		typedef typename std::result_of<FuncType(typename PtrType<ObjType>::type, Args...)>::type type;
	};


	template<typename FuncType, typename... Args>
	struct FuncRetType {
		typedef typename std::result_of<FuncType(Args...)>::type type;
	};

};

template<size_t... IDX_ARRAY>
struct ArgIdxDef {
};

template<typename IdxDefType, typename... RemainArgs>
struct MakeArgIdxBase {
	typedef IdxDefType type;
};

template<size_t... _Idxs, typename Arg, typename... RemainArgs>
struct MakeArgIdxBase<ArgIdxDef<_Idxs...>, Arg, RemainArgs...>
	: MakeArgIdxBase<ArgIdxDef<sizeof...(RemainArgs), _Idxs...>, RemainArgs...> {
};


template<typename... Args2>
struct MakeArgIdx : MakeArgIdxBase<ArgIdxDef<>, Args2...> {
};

template<typename RetType>
struct FuncRetHolder {
	typedef RetType type;
	typedef typename std::add_lvalue_reference<type>::type lvalue_type;

	FuncRetHolder() {
		m_pValue = CMemoryPool::instance()->Allocate(sizeof(type));
	}

	~FuncRetHolder() {
		CMemoryPool::instance()->Recycle(m_pValue, sizeof(type));
	}

	inline void set(type&& value) {
		new (m_pValue)RetType(value);
	}

	inline RetType& Get() const	{
		return *(RetType*)m_pValue;
	}

	void* m_pValue;
};

template<>
struct FuncRetHolder<void> {
	typedef int type;
	typedef int& lvalue_type;

	FuncRetHolder() {
		m_pValue = (int*)(void*)(&m_pValue);
	}

	inline lvalue_type Get() const {
		return *m_pValue;
	}

	int* m_pValue;
};

class IFuncBinder {
public:
	virtual ~IFuncBinder() {
	}
	virtual void Invoke() {
	}
};

//用于子协程的回调
template<typename ObjPtrType, typename FuncType, typename... Args>
class ObjFuncBinder : public IFuncBinder {
public:
	typedef std::tuple<Args&...> TupleType;
	typedef typename FuncBindTypeHelper::ObjFuncRetType<ObjPtrType, FuncType, Args...>::type RetType;
	typedef FuncRetHolder<RetType> RetHolderType;

protected:
	template<int N, typename RetType>
	struct STInvokeAgent {
		template<size_t... IDX_ARRAY>
		static void Invoke(RetHolderType& stRetHolder, ObjPtrType pObj, FuncType pFunc, TupleType& stTuple, ArgIdxDef<IDX_ARRAY...>&&) {
			stRetHolder.set((pObj->*pFunc)(std::get<IDX_ARRAY>(stTuple)...));
		}
	};


	template<int N>
	struct STInvokeAgent<N, void> {
		template<size_t... IDX_ARRAY>
		static void Invoke(RetHolderType& stRetHolder, ObjPtrType pObj, FuncType pFunc, TupleType& stTuple, ArgIdxDef<IDX_ARRAY...>&&) {
			(pObj->*pFunc)(std::get<IDX_ARRAY>(stTuple)...);
		}
	};

public:
	ObjFuncBinder(ObjPtrType pObj, FuncType pFunc, Args&... args)
		: m_pObj(pObj)
		, m_pFunc(pFunc)
		, m_stTuple(args...) {
	}

	virtual ~ObjFuncBinder() {
	}

	virtual void Invoke() {
		STInvokeAgent<0, RetType>::Invoke(m_stRetHolder, m_pObj, m_pFunc, m_stTuple, typename MakeArgIdx<Args...>::type());
	}

	typename RetHolderType::lvalue_type GetRet() {
		return m_stRetHolder.Get();
	}

protected:
	ObjPtrType m_pObj;
	FuncType m_pFunc;
	TupleType m_stTuple;
	RetHolderType m_stRetHolder;
};  
}