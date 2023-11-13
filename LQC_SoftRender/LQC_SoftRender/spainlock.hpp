#pragma once
#include <atomic>

class SpinLock {
public:
	SpinLock() :flag_(false)
	{

	}
	//��ָ��һ���߳��ڻ�ȡ����ʱ��������Ѿ��������̻߳�ȡ��
	/*��ô���߳̽�ѭ���ȴ������ϳ��Ի�ȡ����ֱ����ȡ�����Ż��˳�ѭ��*/
	void lock()
	{
		bool expect = false;
		// ���flag��expect��ֵ��������򷵻�true���Ұ�flag��Ϊtrue��
		//��������򷵻�false���Ұ�expect��Ϊtrue(����ÿ��ѭ��һ��Ҫ��expect��ԭ)
		//a.compare_exchange_weak(b,c)����a�ǵ�ǰֵ��b����ֵ��c��ֵ
		//a == bʱ�����������棬����c��ֵ��a
		//a != bʱ���������ؼ٣�����a���Ƹ�b
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