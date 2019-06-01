#ifndef __IRR_AABB_SIMD_2D_H_INCLUDED__
#define __IRR_AABB_SIMD_2D_H_INCLUDED__

#include "irr/core/math/irrMath.h"
#include "irr/core/memory/memory.h"
#include "irr/core/alloc/AlignedBase.h"

#include <iostream>

#define SIGN_FLIP_MASK_XY	core::vector3du32_SIMD(0x80000000, 0x80000000, 0x00000000, 0x00000000)

namespace irr { namespace core {


class aabb2dSIMDf
{
public:
	inline aabb2dSIMDf()
		: internalBoxRepresentation(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX) {};

	inline explicit aabb2dSIMDf(const core::vectorSIMDf& other)
		: internalBoxRepresentation(other ^ SIGN_FLIP_MASK_XY) {};

	inline void addBox(const aabb2dSIMDf& other)
	{
		internalBoxRepresentation = core::max_(internalBoxRepresentation, other.internalBoxRepresentation);
	}

	inline void addPoint(const core::vectorSIMDf& point)
	{
		addBox(aabb2dSIMDf(point));
	}

	inline core::vectorSIMDf getMinEdge() const
	{
		core::vectorSIMDf minEdge = internalBoxRepresentation;
		minEdge.makeSafe2D();

		return minEdge;
	}

	inline core::vectorSIMDf getMaxEdge() const
	{
		core::vectorSIMDf maxEdge = internalBoxRepresentation.zwzw();
		maxEdge.makeSafe2D();

		return maxEdge;
	}

	//! Resets the bounding box to a one-point box.
	/** \param x X coord of the point.
	\param y Y coord of the point. */
	inline void reset(const core::vectorSIMDf& point)
	{
		internalBoxRepresentation = point.xyxy();
		internalBoxRepresentation ^= SIGN_FLIP_MASK_XY;
	}

	//! Get center of the bounding box
	/** \return Center of the bounding box. */
	inline core::vectorSIMDf getCenter() const
	{
		return (internalBoxRepresentation.zwzw() - internalBoxRepresentation) * vectorSIMDf(0.5f, 0.5f, 0.f, 0.f);
	}


	//! Get extent of the box (maximal distance of two points in the box)
	/** \return Extent of the bounding box. */
	inline core::vectorSIMDf getExtent() const
	{
		core::vectorSIMDf extent = internalBoxRepresentation.zwzw() + internalBoxRepresentation;
		extent.makeSafe2D();

		return extent;
	}

	//! Check if the box is empty.
	/** This means that there is no space between the min and max edge.
	\return True if box is empty, else false. */
	inline bool isEmpty() const
	{
		return ((internalBoxRepresentation ^ SIGN_FLIP_MASK_XY) == internalBoxRepresentation.zwzw()).all();
	}

	//! Get the surface area of the box in squared units
	inline float getArea() const
	{
		core::vectorSIMDf a = internalBoxRepresentation + internalBoxRepresentation.zwxx();
		return a.x * a.y;
	}

	//! Stores all 4 edges of the box into an array
	/**
	A ----- D
	|		|
	|		|
	B ----- C

	array modified by this function will store edges in this order:
	AB, BC, AD, DC
	\param edges: Pointer to array of 4 edges. */
	void getEdges(core::vectorSIMDf* edges) const
	{
		core::vectorSIMDf ordRepBox = internalBoxRepresentation ^ SIGN_FLIP_MASK_XY;

		
		edges[0] = ordRepBox.wxxy();	//AB
		edges[1] = ordRepBox.xyzy();	//BC
		edges[2] = ordRepBox.xwzw();	//AD
		edges[3] = ordRepBox.zwzy();	//DC
	}

	//! Repairs the box.
	/** Necessary if for example MinEdge and MaxEdge are swapped. */
	inline core::aabb2dSIMDf& repair()
	{
		reset(getMinEdge());
		addPoint(getMaxEdge());

		return *this;
	}

	//! Determines if a point is within this box.
	/** Border is included (IS part of the box)!
	\param p: Point to check.
	\return True if the point is within the box and false if not */
	inline bool isPointInside(const core::vectorSIMDf & p) const
	{
		core::vector4db_SIMD result = (p.xyxy() ^ SIGN_FLIP_MASK_XY) <= internalBoxRepresentation;
		return result.allBits();
	}

	//! Determines if a point is within this box and not its borders.
	/** Border is excluded (NOT part of the box)!
	\param p: Point to check.
	\return True if the point is within the box and false if not. */
	inline bool isPointTotalInside(const core::vectorSIMDf & p) const
	{
		core::vector4db_SIMD result = (p.xyxy() ^ SIGN_FLIP_MASK_XY) < internalBoxRepresentation;
		return result.allBits();
	}

	//! Check if this box is completely inside the 'other' box.
	/** \param other: Other box to check against.
	\return True if this box is completly inside the other box,
	otherwise false. */
	inline bool isFullInside(const aabb2dSIMDf & other) const
	{
		core::vector4db_SIMD a = internalBoxRepresentation <= other.internalBoxRepresentation;
		return a.allBits();
	}

	//! Determines if the axis-aligned box intersects with another axis-aligned box.
	/** \param other: Other box to check a intersection with.
	\return True if there is an intersection with the other box,
	otherwise false. */
	inline bool intersectsWithBox(const aabb2dSIMDf& other) const
	{
		//will privide better algorithm 

		core::vectorSIMDf tmp = other.internalBoxRepresentation ^ SIGN_FLIP_MASK_XY;

		return
			isPointInside(tmp) ||
			isPointInside(tmp.zwxx()) ||
			isPointInside(tmp.xwxx()) ||
			isPointInside(tmp.zyxx());
	}

private:
	core::vectorSIMDf internalBoxRepresentation;
};


} 
}

#undef SIGN_FLIP_MASK_XY

#endif
