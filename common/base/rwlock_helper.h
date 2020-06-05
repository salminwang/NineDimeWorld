#pragma once
#include <pthread.h>

namespace Astra {

class RWLock {
private:
	pthread_rwlock_t m_lock;
	bool m_inited;

public:
	RWLock(bool auto_init = false) {
		m_inited = false;
		if (auto_init) {
			Init();
		}
	}

	~RWLock() {
		if (m_inited) {
			m_inited = false;
			pthread_rwlock_destroy(&m_lock);
		}
	}

	bool Init() {
		if (m_inited) {
			return true;
		}

		m_inited = (0 == pthread_rwlock_init(&m_lock, NULL));
		return m_inited;
	}

	void RLock() {
		pthread_rwlock_rdlock(&m_lock);
	}

	void WLock() {
		pthread_rwlock_wrlock(&m_lock);
	}

	void Unlock() {
		pthread_rwlock_unlock(&m_lock);
	}
};


class Lock {
private:
	pthread_mutex_t m_mutex;
	bool m_inited;

public:
	Lock() {
		m_inited = false;
	}

	~Lock() {
		if (m_inited) {
			m_inited = false;
			pthread_mutex_destroy(&m_mutex);
		}
	}

	bool Init() {
		if (m_inited) {
			return true;
		}

		m_inited = (0 == pthread_mutex_init(&m_mutex, NULL));
		return m_inited;
	}

	void AddLock() {
		pthread_mutex_lock(&m_mutex);
	}

	void Unlock() {
		pthread_mutex_unlock(&m_mutex);
	}
};


class LockAgent {
public:
	LockAgent(Lock& lock) {
		m_lock = &lock;
		m_lock->AddLock();
	}

	~LockAgent() {
		m_lock->Unlock();
	}

protected:
	Lock* m_lock;
};


class RWLockAgent {
public:
	RWLockAgent(RWLock& rwlock, bool b_lock) {
		m_rwlock = &rwlock;
		if (b_lock) {
			m_rwlock->RLock();
		} else {
			m_rwlock->WLock();
		}
	}

	~RWLockAgent() {
		m_rwlock->Unlock();
	}

protected:
	RWLock* m_rwlock;

};

}
