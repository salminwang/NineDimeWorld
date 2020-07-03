#pragma once
#include <map>
#include <stdint.h>
#include "base/singleton.h"

namespace Astra {

class MemoryPool : public Singleton<MemoryPool, true> {
public:
	static const size_t BRACKET_SIZE = 8;
    struct NodeBase {
		NodeBase* m_next_;
	};

    struct NodeInfo {
        uint32_t m_nodenum_;
		NodeBase* m_firstnode_;
    };

    typedef std::map<size_t, NodeInfo> NodeMapType;

public:
    MemoryPool() {
	}

    ~MemoryPool() {
		for (auto iter = m_node_map_.begin(); iter != m_node_map_.end(); ++iter) {
			NodeBase* pNode = iter->second.m_firstnode_;
			while (NULL != pNode)
			{
				NodeBase* pTmp = pNode;
				pNode = pNode->m_next_;
				free(pTmp);
			}
		}

		m_node_map_.clear();
	}

    void* Allocate(size_t size) {
		if(0 == size) {
			assert(0);
			return NULL;
		}

		if(1 != BRACKET_SIZE) {
			--size;
			size = size - (size % BRACKET_SIZE) + BRACKET_SIZE;
		}

		if (size < sizeof(NodeBase)) {
			size = sizeof(NodeBase);
		}

		NodeInfo* pInfo = NULL;

		auto iter = m_node_map_.find(size);
		if (iter == m_node_map_.end()) {
			pInfo = &m_node_map_[size];
			pInfo->m_nodenum_ = 0;
			pInfo->m_firstnode_ = NULL;
		} else {
			pInfo = &(iter->second);
		}

		void* pRet = NULL;

		if(NULL == pInfo->m_firstnode_) {
			pRet = malloc(size);
		} else {
			pRet = pInfo->m_firstnode_;
			pInfo->m_firstnode_ = pInfo->m_firstnode_->m_next_;
			if(pInfo->m_nodenum_ > 0) {
				--pInfo->m_nodenum_;
			}
		}

		return pRet;
	}

    void Recycle(void* ptr, size_t size) {
		if(0 == size) {
			assert(0);
			return;
		}

		if(1 != BRACKET_SIZE) {
			-- size;
			size = size - (size % BRACKET_SIZE) + BRACKET_SIZE;
		}

		if (size < sizeof(NodeBase)) {
			size = sizeof(NodeBase);
		}

		auto iter = m_node_map_.find(size);
		if (iter == m_node_map_.end()) {
			assert(0);
			return;
		}

		NodeInfo* pInfo = &(iter->second);
		NodeBase* pNode = (NodeBase*)ptr;
		pNode->m_next_ = pInfo->m_firstnode_;
		pInfo->m_firstnode_ = pNode;
	}

    template<typename Type, typename... Args>
	inline Type* New(Args&&... args) {
		void* ptr = Allocate(sizeof(Type));
		if (NULL == ptr) {
			return NULL;
		}

		new (ptr)Type(args...);
		return (Type*)ptr;
	}

	template<typename Type>
	inline Type* New() {
		void* ptr = Allocate(sizeof(Type));
		if (NULL == ptr) {
			return NULL;
		}
		new (ptr)Type;
		return (Type*)ptr;
	}

	template<typename Type>
	inline void Delete(Type* ptr) {
		ptr->~Type();
		Recycle((void*)ptr, sizeof(Type));
	}

protected:
	NodeMapType m_node_map_;   
};

#define POOLED_NEW(Type, ...) MemoryPool::instance()->New<Type>(__VA_ARGS__)
#define POOLED_DELETE(ptr) MemoryPool::instance()->Delete(ptr)

}