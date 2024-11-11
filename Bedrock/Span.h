// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <Bedrock/Core.h>
#include <Bedrock/TypeTraits.h>
#include <Bedrock/Algorithm.h>

template <typename taType>
struct Span
{
	using ElementType = taType;
	using ValueType = RemoveCV<taType>;

	constexpr Span()						= default;
	constexpr Span(const Span&)				= default;
	constexpr Span(Span&&)					= default;
	constexpr ~Span()						= default;
	constexpr Span& operator=(const Span&)	= default;
	constexpr Span& operator=(Span&&)		= default;

	// Construction from pointers
	constexpr Span(taType* inElems, int inSize) : mData(inElems), mSize(inSize) {}
	constexpr Span(taType* inBegin, taType* inEnd) : mData(inBegin), mSize((int)(inEnd - inBegin)) { gAssert(inEnd >= inBegin); }

	// Construction from array
	template <int taSize>
	constexpr Span(taType (&inElems)[taSize]) : mData(inElems), mSize(taSize) {}

	// Construction from Span of a different type (for conversion to const among other things)
	template <typename taOtherType>
	requires cIsConvertible<taOtherType (*)[], taType (*)[]>
	constexpr Span(const Span<taOtherType>& inOtherSpan) : mData(inOtherSpan.Begin()), mSize(inOtherSpan.Size()) {}

	// Constructor from containers
	template <typename taContainer>
	requires cIsContiguous<RemoveCV<taContainer>> && cIsConvertible<typename taContainer::ValueType (*)[],  taType (*)[]>
	constexpr Span(taContainer& inContainer) : mData(inContainer.Begin()), mSize(inContainer.Size()) {}

	constexpr int Size() const { return mSize; }
	constexpr bool Empty() const { return mSize == 0; }

	constexpr const taType* Begin() const { return mData; }
	constexpr const taType* End() const { return mData + mSize; }
	constexpr const taType* begin() const { return mData; }
	constexpr const taType* end() const { return mData + mSize; }
	constexpr taType* Begin() { return mData; }
	constexpr taType* End() { return mData + mSize; }
	constexpr taType* begin() { return mData; }
	constexpr taType* end() { return mData + mSize; }

	constexpr taType& operator[](int inPosition) const { gBoundsCheck(inPosition, mSize); return mData[inPosition]; }

	constexpr bool operator==(Span inOther) const { return gEquals(*this, inOther); }
	template <typename taOtherType>
	requires cIsConvertible<taOtherType (*)[], taType (*)[]>
	constexpr bool operator==(Span<taOtherType> inOther) const { return gEquals(*this, inOther); }

	constexpr Span First(int inCount) const { gBoundsCheck(inCount, mSize); return { mData, inCount }; }
	constexpr Span Last(int inCount) const { gBoundsCheck(inCount, mSize); return { mData + mSize - inCount, inCount }; }
	constexpr Span SubSpan(int inPosition, int inCount = cMaxInt) const;

private:
	taType* mData = nullptr;
	int		mSize = 0;
};


// Span is a contiguous container.
template<class T> inline constexpr bool cIsContiguous<Span<T>> = true;


template <typename taType>
constexpr Span<taType> Span<taType>::SubSpan(int inPosition, int inCount) const
{
	gBoundsCheck(inPosition, mSize);
	int size = gMin(inCount, mSize - inPosition);
	return { mData + inPosition, size };
}