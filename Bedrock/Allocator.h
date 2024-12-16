// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <Bedrock/Memory.h>
#include <Bedrock/TempMemory.h>


// Default allocator. Allocates from the heap.
template <typename taType>
struct DefaultAllocator
{
	// Allocate memory.
	static taType*	Allocate(int inSize)				{ return (taType*)gMemAlloc(inSize * sizeof(taType)).mPtr; }
	static void		Free(taType* inPtr, int inSize)		{ gMemFree({ (uint8*)inPtr, inSize * (int64)sizeof(taType) }); }

	// Try changing the size of an existing allocation, return false if unsuccessful.
	static bool		TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize)	{ gAssert(inPtr != nullptr); return false; }
};



// Temp memory allocator. Allocates from a thread-local global MemArena.
// Falls back to the default allocator if temp memory runs out.
template <typename taType>
struct TempAllocator
{
	// Allocate memory.
	static taType*	Allocate(int inSize);
	static void     Free(taType* inPtr, int inSize);

	// Try changing the size of an existing allocation, return false if unsuccessful.
	static bool		TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize);
};


// Allocates from an externally provided MemArena.
template <typename taType, int taMaxPendingFrees>
struct ArenaAllocatorBase
{
	using MemArenaType = MemArena<taMaxPendingFrees>;

	ArenaAllocatorBase() = default;
	ArenaAllocatorBase(MemArenaType& inArena) : mArena(&inArena) {}

	// Allocate memory.
	taType*			Allocate(int inSize)				{ return (taType*)mArena->Alloc(inSize * sizeof(taType)).mPtr; }
	void			Free(taType* inPtr, int inSize)	{ mArena->Free({ (uint8*)inPtr, inSize * (int64)sizeof(taType) }); }

	// Try changing the size of an existing allocation, return false if unsuccessful.
	bool			TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize);

	MemArenaType*		GetArena()							{ return mArena; }
	const MemArenaType* GetArena() const					{ return mArena; }

private:
	MemArenaType*		mArena = nullptr;
};

// Shorter version with default number of allowed out of order frees.
// This alias is needed because containers only accept alloctor with a single template parameter.
template <typename taType>
using ArenaAllocator = ArenaAllocatorBase<taType, cDefaultMaxPendingFrees>;


// Allocates from an internal VMemArena which uses virtual memory.
// The VMemArena can grow as necessary by committing more virtual memory.
template <typename taType>
struct VMemAllocator
{
	using VMemArenaType = VMemArena<0>; // Don't need to support any out of order free since the arena isn't shared.

	static constexpr int64 cDefaultReservedSize = VMemArenaType::cDefaultReservedSize; // By default the arena will reserve that much virtual memory.
	static constexpr int64 cDefaultCommitSize   = VMemArenaType::cDefaultCommitSize;   // By default the arena will commit that much virtual memory every time it grows.

	VMemAllocator() = default;
	VMemAllocator(int inReservedSizeInBytes, int inCommitIncreaseSizeInBytes = cDefaultCommitSize)
		: mArena(inReservedSizeInBytes, inCommitIncreaseSizeInBytes) {}

	// Allocate memory.
	taType*			Allocate(int inSize);
	void			Free(taType* inPtr, int inSize)	{ mArena.Free({ (uint8*)inPtr, inSize * (int64)sizeof(taType) }); }

	// Try changing the size of an existing allocation, return false if unsuccessful.
	bool			TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize);

	const VMemArenaType* GetArena() const			{ return &mArena; }

private:
	VMemArenaType	mArena;
};




template <typename taType>
taType* TempAllocator<taType>::Allocate(int inSize)
{
	MemBlock mem = gTempMemArena.Alloc(inSize * sizeof(taType));

	if (mem != nullptr) [[likely]]
		return (taType*)mem.mPtr;

	return DefaultAllocator<taType>::Allocate(inSize);
}


template <typename taType>
void TempAllocator<taType>::Free(taType* inPtr, int inSize)
{
	if (gTempMemArena.Owns(inPtr)) [[likely]]
		gTempMemArena.Free({ (uint8*)inPtr, inSize * (int64)sizeof(taType) });
	else
		DefaultAllocator<taType>::Free(inPtr, inSize);
}


template <typename taType>
bool TempAllocator<taType>::TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize)
{
	gAssert(inPtr != nullptr); // Call Allocate instead.

	if (gTempMemArena.Owns(inPtr)) [[likely]]
	{
		MemBlock mem = { (uint8*)inPtr, inCurrentSize * (int64)sizeof(taType) };
		return gTempMemArena.TryRealloc(mem, inNewSize * sizeof(taType));
	}
	
	return DefaultAllocator<taType>::TryRealloc(inPtr, inCurrentSize, inNewSize);
}


template <typename taType, int taMaxPendingFrees>
bool ArenaAllocatorBase<taType, taMaxPendingFrees>::TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize)
{
	gAssert(inPtr != nullptr); // Call Allocate instead.

	MemBlock mem = { (uint8*)inPtr, inCurrentSize * (int64)sizeof(taType) };
	return mArena->TryRealloc(mem, inNewSize * sizeof(taType));
}


template <typename taType>
taType* VMemAllocator<taType>::Allocate(int inSize)
{
	// If the arena wasn't initialized yet, do it now (with default values).
	// It's better to do it lazily than reserving virtual memory in every container default constructor.
	if (mArena.GetMemBlock() == nullptr) [[unlikely]]
		mArena = VMemArenaType(cDefaultReservedSize, cDefaultCommitSize);

	return (taType*)mArena.Alloc(inSize * sizeof(taType)).mPtr;
}


template <typename taType>
bool VMemAllocator<taType>::TryRealloc(taType* inPtr, int inCurrentSize, int inNewSize)
{
	gAssert(inPtr != nullptr); // Call Allocate instead.

	MemBlock mem = { (uint8*)inPtr, inCurrentSize * (int64)sizeof(taType) };
	return mArena.TryRealloc(mem, inNewSize * sizeof(taType));
}
