#pragma once
#include <atomic>

class SpinLock {
public:
	SpinLock() :flag_(false)
	{

	}
	//是指当一个线程在获取锁的时候，如果锁已经被其它线程获取，
	/*那么该线程将循环等待，不断尝试获取锁，直到获取到锁才会退出循环*/
	void lock()
	{
		bool expect = false;
		// 检查flag和expect的值，若相等则返回true，且把flag置为true，
		//若不相等则返回false，且把expect置为true(所以每次循环一定要将expect复原)
		//a.compare_exchange_weak(b,c)其中a是当前值，b期望值，c新值
		//a == b时：函数返回真，并把c赋值给a
		//a != b时：函数返回假，并把a复制给b
		while (!flag_.compare_exchange_weak(expect, true))
		{
			expect = false;
		}

	}

	void unlock()
	{
		flag_.store(false);
	}


private:
	std::atomic<bool> flag_;

};