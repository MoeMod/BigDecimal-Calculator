// 【小白白】
// 高精度加减乘除模幂根

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// 模板计算工具
template<unsigned x>
struct template_bits_high
{
	static const unsigned value = template_bits_high< (x >> 1) >::value + 1;
};
template<>
struct template_bits_high<1>
{
	static const unsigned value = 0;
};
template<unsigned x>
struct template_bits_high_remove
{
	static const unsigned value = (x & ~(1<<(template_bits_high<x>::value)));
};
template<unsigned x>
struct template_bits_is_power_of_2
{
	static const bool value = !(x & (x-1) );
};

inline unsigned BitsGetHigh(unsigned x) // log_2(n)??
{
	/*unsigned int &v=x;	         // 32-bit value to find the log2 of
	register unsigned int r; // result of log2(v) will go here
	register unsigned int shift;

	r = (v > 0xFFFF) << 4; v >>= r;
	shift = (v > 0xFF) << 3; v >>= shift; r |= shift;
	shift = (v > 0xF) << 2; v >>= shift; r |= shift;
	shift = (v > 0x3) << 1; v >>= shift; r |= shift;
	r |= (v >> 1);
	return r;*/

	__asm
	{
		bsr eax, x
	}
}

inline unsigned BitsGetLow(unsigned x)
{
	/*register unsigned r = 0;
	for (int i = 4; i >= 0; i--)
		if ((x & ((1 << (1 << i)) - 1)) == 0)
		{
			x >>= (1 << i);
			r += (1 << i);
		}
	return r;*/
	__asm
	{
		bsf eax, x
	}
}

inline char add_carry(unsigned char chFlagCF, unsigned a, unsigned b, unsigned &Result)
{
	return _addcarry_u32(chFlagCF, a, b, &Result);
	//Result = a + b + (chFlagCF&1);
	//return Result < a || Result < b;
}

inline char sub_borrow(unsigned char chFlagCF, unsigned a, unsigned b, unsigned &Result)
{
	return _subborrow_u32(chFlagCF, a, b, &Result);
	//Result = a - b - (chFlagCF & 1);
	//return (a<b);
}

class CBigInt
{
public:
	CBigInt() : m_sign(false)
	{
		m_Ints.push_back(0);
	}
	explicit CBigInt(unsigned x) : m_sign(false)
	{
		m_Ints.push_back(x);
	}
	explicit CBigInt(unsigned long long x) : m_sign(false)
	{
		m_Ints.push_back(unsigned(x & 0xFFFFFFFF));
		if (unsigned(x >> 32))
			m_Ints.push_back(unsigned(x >> 32));
	}
	explicit CBigInt(const string &other) : CBigInt()
	{
		for (char ch : other)
		{
			if (ch<'0' || ch>'9')
				continue;
			muln<10>();
			add(ch - '0');
		}
	}

	CBigInt &operator +=(const CBigInt &other)
	{
		if (this == &other)
		{
			shl(1);
			return *this;
		}
		if (is_zero())
		{
			return *this = other;
		}
		if (other.is_zero())
		{
			return *this;
		}
		if (other.m_sign)
		{
			CBigInt &inverse = const_cast<CBigInt &>(other);
			inverse.m_sign = false;
			*this -= inverse;
			inverse.m_sign = true;
			return *this;
		}
		if (m_sign)
		{
			m_sign = false;
			*this -= other;
			m_sign = true;
			if (is_zero())
				m_sign = false;
			return *this;
		}
		
		int count1 = m_Ints.size();
		unsigned * const begin1 = m_Ints.data();
		int count2 = other.m_Ints.size();
		const unsigned * const begin2 = other.m_Ints.data();
		if (count1 < count2)
		{ 
			count1 = count2;
			m_Ints.resize(count2, 0);
		}
		unsigned char chFlagCF = 0;
		for (int i = 0; i < count2; i++)
		{
			chFlagCF = add_carry(chFlagCF, begin1[i], begin2[i], begin1[i]);
		}
		// 继续进位
		if (chFlagCF)
		{
			if (count1 > count2)
			{ 
				chFlagCF = add_carry(0, m_Ints[count2], 1, m_Ints[count2]);
				if (chFlagCF)
					m_Ints.push_back(1);
			}
			else
			{
				m_Ints.push_back(1);
			}
		}
		return *this;
	}

	CBigInt &operator -=(const CBigInt &other)
	{
		if (this == &other)
		{
			reset();
			return *this;
		}
		if (is_zero())
		{
			*this = other;
			m_sign ^= 1;
			return *this;
		}
		if (other.is_zero())
		{
			return *this;
		}
		if (other.m_sign)
		{
			CBigInt &inverse = const_cast<CBigInt &>(other);
			inverse.m_sign = false;
			*this += inverse;
			inverse.m_sign = true;
			return *this;
		}
		if (m_sign)
		{
			m_sign = false;
			*this += other;
			m_sign = true;
			if (is_zero())
				m_sign = false;
			return *this;
		}

		int count1 = m_Ints.size();
		unsigned * const begin1 = m_Ints.data();
		int count2 = other.m_Ints.size();
		const unsigned * const begin2 = other.m_Ints.data();
		if (count1 < count2)
		{
			count1 = count2;
			m_Ints.resize(count2, 0);
		}
		unsigned char chFlagCF = 0;
		for (int i = 0; i < count2; i++)
		{
			chFlagCF = sub_borrow(chFlagCF, begin1[i], begin2[i], begin1[i]);
		}
		if (chFlagCF)
		{
			if (count1 > count2)
			{
				chFlagCF = sub_borrow(0, m_Ints[count2], 1, m_Ints[count2]);
				if (chFlagCF)
				{
					for (auto &x : m_Ints)
						x = ~x + 1;
					m_sign = true;
				}
			}
			else
			{
				for (auto &x : m_Ints)
					x = ~x + 1;
				m_sign = true;
			}
		}
		while (!m_Ints.empty() && !*m_Ints.crbegin())
			m_Ints.pop_back();
		if (!m_Ints.size())
			reset();
		return *this;
	}

	CBigInt &operator *=(const CBigInt &other)
	{
		if (this == &other)
		{
			auto new_this = *this;
			return *this *= new_this;
		}
		if (is_zero() || other.is_zero())
		{
			reset();
			return *this;
		}
		
		auto backup_this = *this;
		reset();
		//for (unsigned i = 0; i < other.m_Ints.size();i++)
		for (auto iter = other.m_Ints.crbegin(); iter != other.m_Ints.crend(); iter++)
		{
			shl(32);
			CBigInt Temp = backup_this;
			Temp.mul(*iter);
			
			*this += Temp;
		}
		m_sign ^= other.m_sign;
		return *this;
	}
	CBigInt &operator /=(const CBigInt &other)
	{
		return *this = div_other(other).first;
	}
	CBigInt &operator %=(const CBigInt &other)
	{
		return *this = div_other(other).second;
	}
	std::pair<const CBigInt, const CBigInt> div_other(const CBigInt &other) const
	{
		if (other.is_zero())
		{
			throw logic_error("Invalid argument: divide by zero");
		}
		if (is_zero())
		{
			return make_pair(CBigInt(), CBigInt());
		}
		if (this == &other)
		{
			return make_pair(CBigInt(1u), CBigInt());
		}

		CBigInt dividend = *this;

		int LogQuotient = dividend.BitsGetHigh() - other.BitsGetHigh();
		if (LogQuotient < 0)
		{
			return make_pair(CBigInt(), other);
		}

		CBigInt Result;

		for (int i = LogQuotient; i >= 0; i--)
		{
			if (dividend.is_zero())
				break;
			CBigInt Test = other;
			Test.shl(i);
			CBigInt NewThis = dividend - Test;
			if (!NewThis.m_sign)
			{
				dividend = NewThis;
				Result.BitsSet(i);
			}
		}
		
		return make_pair(Result, dividend);
	}
	const CBigInt operator +(const CBigInt &other) const
	{
		CBigInt Result = *this;
		Result += other;
		return std::move(Result);
	}
	const CBigInt operator -(const CBigInt &other) const
	{
		CBigInt Result = *this;
		Result -= other;
		return std::move(Result);
	}
	const CBigInt operator *(const CBigInt &other) const
	{
		CBigInt Result = *this;
		Result *= other;
		return std::move(Result);
	}
	const CBigInt operator /(const CBigInt &other) const
	{
		CBigInt Result = *this;
		Result /= other;
		return std::move(Result);
	}
	const CBigInt operator %(const CBigInt &other) const
	{
		CBigInt Result = *this;
		Result %= other;
		return std::move(Result);
	}
	bool operator==(const CBigInt &other) const
	{
		if (m_sign != other.m_sign)
			return false;
		if (m_Ints.size() != other.m_Ints.size())
			return false;
		for (int i = m_Ints.size() - 1; i >= 0; i--)
		{
			if (m_Ints[i] != other.m_Ints[i])
				return false;
		}
		return true;
	}
	bool operator!=(const CBigInt &other) const
	{
		return !(*this == other);
	}
	bool operator<(const CBigInt &other) const
	{
		if (m_sign != other.m_sign)
		{
			return m_sign;
		}
		if (m_Ints.size() != other.m_Ints.size())
		{
			return m_Ints.size() < other.m_Ints.size();
		}
		for (int i = m_Ints.size() - 1; i >= 0; i--)
		{
			if (m_Ints[i] == other.m_Ints[i])
				continue;
			return m_Ints[i] < other.m_Ints[i];
		}
		return false;
	}
	bool operator<=(const CBigInt &other) const
	{
		return (*this == other) || (*this < other);
	}
	bool operator>(const CBigInt &other) const
	{
		return !(*this <= other);
	}
	bool operator>=(const CBigInt &other) const
	{
		return !(*this < other);
	}
	void BitsSet(unsigned n)
	{
		unsigned x = n >> 5;
		unsigned y = n & 31;
		if (x >= m_Ints.size())
		{
			m_Ints.resize(x + 1, 0);
		}
		m_Ints[x] |= (1u << y);
	}
	void BitsUnSet(unsigned n)
	{
		unsigned x = n >> 5;
		unsigned y = n & 31;
		if (x >= m_Ints.size())
		{
			return;
		}
		m_Ints[x] &= ~(1u << y);
	}
	bool BitsGet(unsigned n) const
	{
		unsigned x = n >> 5;
		unsigned y = n & 31;
		if (x >= m_Ints.size())
		{
			return false;
		}
		return !!(m_Ints[x] & (1u << y));
	}
	int BitsGetHigh() const
	{
		for (int x = m_Ints.size() - 1; x >= 0; x--)
		{
			if (m_Ints[x])
				return x * 32 + ::BitsGetHigh(m_Ints[x]);
		}
		return -1;
	}
	int BitsGetLow() const
	{
		for (int x = 0; x < (int)m_Ints.size(); x++)
		{
			if (m_Ints[x])
				return x * 32 + ::BitsGetLow(m_Ints[x]);
		}
		return -1;
	}
	void neg()
	{
		m_sign = !m_sign;
	}
	void add(unsigned n)
	{
		if (m_sign)
		{
			m_sign = false;
			sub(n);
			m_sign = true;
			if (is_zero())
				m_sign = false;
			return;
		}
		unsigned char chFlagCF = 0;
		chFlagCF = add_carry(chFlagCF, m_Ints[0], n, m_Ints[0]);
		if (chFlagCF)
		{
			if (m_Ints.size() == 1)
				m_Ints.push_back(1);
			else
				m_Ints[1] += 1;
		}
	}
	void sub(unsigned n)
	{
		if (m_sign)
		{
			m_sign = false;
			add(n);
			m_sign = true;
			if (is_zero())
				m_sign = false;
			return;
		}
		unsigned char chFlagCF = 0;
		chFlagCF = sub_borrow(chFlagCF, m_Ints[0], n, m_Ints[0]);
		if (chFlagCF)
		{
			if (m_Ints.size() == 1)
			{
				m_Ints[0] = 0 - m_Ints[0];
				m_sign = true;
			}
			else
			{
				m_Ints[1] -= 1;
				if (!m_Ints[1] && m_Ints.size() == 2)
					m_Ints.pop_back();
			}
		}
	}
	void shl(unsigned n)
	{
		if (is_zero() || !n)
			return;
		unsigned remain = n % 32;
		unsigned extend = n / 32;

		if (remain)
		{
			auto count = m_Ints.size();
			unsigned temp = 0;
			for (unsigned i = 0; i < count; i++)
			{
				unsigned prev = temp;
				temp = m_Ints[i];
				m_Ints[i] <<= remain;
				m_Ints[i] |= prev >> (32 - remain);
			}
			if (temp >> (32 - remain))
				m_Ints.push_back(temp >> (32 - remain));
		}

		for (unsigned i = 0; i < extend; i++)
			m_Ints.insert(m_Ints.begin(), 0);
	}
	void shr(unsigned n)
	{
		if (is_zero())
			return;
		unsigned remain = n & 31;
		unsigned extend = n >> 5;

		if (remain)
		{
			auto count = m_Ints.size();
			unsigned temp = 0;
			for (int i = count - 1; i >= 0; i--)
			{
				unsigned prev = temp;
				temp = m_Ints[i];
				m_Ints[i] >>= remain;
				m_Ints[i] |= (prev & ((1 << remain) - 1)) << (32 - remain);
			}
			if (!m_Ints[count - 1])
				m_Ints.pop_back();
		}

		if (extend)
			m_Ints.erase(m_Ints.begin(), m_Ints.begin() + extend);
		if (!m_Ints.size())
			reset();
	}
	void mul(unsigned n)
	{
		auto backup_this = *this;
		reset();
		while (n)
		{
			unsigned i = ::BitsGetHigh(n);
			
			auto this_shl_i = backup_this;
			this_shl_i.shl(i);
			
			*this += this_shl_i;
			
			n &= ~(1<<i);
		}
		/*unsigned long long multiplier = n;
		for (unsigned int i = 0; i < backup_this.m_Ints.size(); ++i)
		{
			CBigInt Temp(multiplier * backup_this.m_Ints[i]);
			Temp.shl(i << 5);
			*this += Temp;
		}*/
	}
	
	template<unsigned n>
	inline void muln()
	{
		auto backup_this = *this;
		reset();
		muln_process<n>(backup_this);
	}
	template<>
	inline void muln<1>() {}
	template<>
	inline void muln<0>()
	{
		reset();
	}
	template<unsigned n>
	inline void muln_process(CBigInt &backup_this)
	{
		auto this_shl_i = backup_this;
		this_shl_i.shl(template_bits_high<n>::value);
		*this += this_shl_i;
		muln_process<template_bits_high_remove<n>::value>(backup_this);
	}
	template<>
	inline void muln_process<0>(CBigInt &backup_this) {}

	/*unsigned div_fast(unsigned n)
	{
		if (!n)
			throw logic_error("Invalid argument: divide by zero");
		if (!(n & (n - 1)))
		{
			shr(::BitsGetLow(n));
			return m_Ints[0] & ((1<<n)-1);
		}
		// http://www.cnblogs.com/shines77/p/4189074.html

		auto backup_this = *this;

		CBigInt multiplier =  0x8000000000000000llu / n; //(1<<63)
		//unsigned multiplier = 0x80000000u / n; //(1<<31)
		*this *= multiplier;
		this->shr(63);

		auto multiplied_this = *this;
		multiplied_this *= n;
		unsigned remainder = (backup_this - multiplied_this).m_Ints[0];
		add(remainder / n);
		return remainder % n;
	}*/
	unsigned div(unsigned n)
	{
		if (!n)
			throw logic_error("Invalid argument: divide by zero");
		if (!(n & (n - 1)))
		{
			shr(::BitsGetLow(n));
			return m_Ints[0] & ((1 << n) - 1);
		}
		unsigned remainder = 0;
		for (auto iter = m_Ints.rbegin(); iter != m_Ints.rend(); iter++)
		{
			unsigned long long by_divisior = (long long(remainder) << 32) | *iter;
			remainder = by_divisior % n;
			*iter = unsigned(by_divisior / n);
		}
		if (!(*m_Ints.rbegin()))
			m_Ints.pop_back();
		if (!m_Ints.size())
			reset();
		return remainder;
	}

	template<unsigned n>
	unsigned divn()
	{
		unsigned remainder = 0;
		for (auto iter = m_Ints.rbegin(); iter != m_Ints.rend(); iter++)
		{
			unsigned long long by_divisior = (long long(remainder) << 32) | *iter;
			remainder = by_divisior % n;
			*iter = unsigned(by_divisior / n);
		}
		if (!(*m_Ints.rbegin()))
			m_Ints.pop_back();
		if (!m_Ints.size())
			reset();
		return remainder;
	}
	template<>
	unsigned divn<0>() = delete;

	void pow2()
	{
		*this *= *this;
	}

	void pown(unsigned n)
	{
		if (!n)
		{
			reset();
			m_Ints[0] = 1;
			return;
		}
		if (n == 1)
		{
			return;
		}
		--n;
		auto backup_this = *this;
		for (unsigned i = 0; i <= ::BitsGetHigh(n);i++)
		{
			if (n & (1 << i))
			{
				*this *= backup_this;
			}
			backup_this *= backup_this;
		}
	}
	const CBigInt pow(unsigned n) const
	{
		CBigInt Result = *this;
		Result.pown(n);
		return std::move(Result);
	}

	const CBigInt sqrt() const
	{
		if (is_zero())
			return CBigInt();
		if (m_sign)
		{
			throw std::logic_error("negative sqrt");
		}
		
		/*
		// 二分查找法
		CBigInt y1(1u);
		y1.shl(BitsGetHigh() >> 1);
		CBigInt y2 = y1;
		y2.shl(1);
		CBigInt y = y1;
		y += y2;
		y.shr(1);

		CBigInt unit(1u);
		do
		{
			CBigInt temp = y;
			temp.pow2();
			if (temp <= *this)
			{
				y1 = y;
				y += y2;
				y.shr(1);
			}
			else
			{
				y2 = y;
				y += y1;
				y.shr(1);
			}
			
		} while (y2 - y1 > unit);
		return y;*/

		/*
		// 牛顿迭代法 ?? 好像不能用
		CBigInt y0(1u);
		y0.shl(BitsGetHigh() / 2);

		CBigInt y1 = *this;
		y1.shr(BitsGetHigh() / 2);
		y1 += y0;
		y1.shr(1);

		while (y1 < y0)
		{
			y0 = y1;
			y1.shl(1);
			y1 += *this / y0;
			y1.shr(1);
		}
		return y0;*/

		// 逐比特确认法
		CBigInt x = *this;
		CBigInt temp;
		unsigned v_bit = BitsGetHigh()/2;
		CBigInt n;
		CBigInt b(1u);
		b.shl(v_bit);
		do{
			//temp = ((n << 1) + b) << (v_bit--);
			temp = n;
			temp.shl(1);
			temp += b;
			temp.shl(v_bit--);
			if (x >= temp)
			{
				n += b;
				x -= temp;
			}
			b.shr(1);
		} while (!b.is_zero());
		return n;
	}

	void reset()
	{
		m_Ints.clear();
		m_Ints.push_back(0);
		m_sign = false;
	}
	bool is_zero() const
	{
		return m_Ints.size() == 1 && !m_Ints[0];
	}
	std::ostream &output_hex(std::ostream &out) const
	{
		if (m_sign)
			out << '-';
		for (auto iter = m_Ints.crbegin(); iter != m_Ints.crend(); ++iter)
		{
			out.fill('0');
			cout.width(8);
			out << std::hex << *iter << " ";
		}
		return out;
	}
	
	std::ostream &output_dec(std::ostream &out) const
	{
		if (is_zero())
			return out << 0;
		if (m_sign)
			out << '-';

		std::vector<unsigned> stack;

		auto new_this = *this;
		unsigned remainder = 0;
		while (!new_this.is_zero())
		{
			remainder = new_this.divn<1000000000>();
			stack.push_back(remainder);
		}

		char backupFill = out.fill();
		auto backupWidth = out.width();
		out.fill('0');
		out << std::dec;
		for (auto iter = stack.crbegin(); iter != stack.crend();iter++)
		{
			out << *iter;
			out.width(9);
		}
		out.fill(backupFill);
		out.width(backupWidth);
		return out;
	}

	friend std::ostream &operator<<(std::ostream &out, const CBigInt &obj)
	{
		return obj.output_dec(out);
	}

private:
	std::vector<unsigned> m_Ints;
	bool m_sign;
};

int main()
{
	string StrA;
	string StrB;
	cout << "[Input]" << endl;
	cout << " A =";
	cin >> StrA;
	cout << " B =";
	cin >> StrB;
	CBigInt DataA(StrA);
	CBigInt DataB(StrB);
	cout << "[Output]" << endl;
	cout << "A+B=" << DataA+DataB << endl;
	cout << "A-B=" << DataA - DataB << endl;
	cout << "A*B=" << DataA*DataB << endl;
	cout << "A/B=" << DataA/DataB << endl;
	cout << "A%B=" << DataA % DataB << endl;
	cout << "sqrt(A)=" << DataA.sqrt() << endl;
	cout << "sqrt(B)=" << DataB.sqrt() << endl;
	cout << endl;
}