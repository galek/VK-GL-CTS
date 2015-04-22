/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Texture utilities.
 *//*--------------------------------------------------------------------*/

#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "deRandom.hpp"
#include "deMath.h"
#include "deMemory.h"

#include <limits>

namespace tcu
{

static inline float sRGBChannelToLinear (float cs)
{
	if (cs <= 0.04045)
		return cs / 12.92f;
	else
		return deFloatPow((cs + 0.055f) / 1.055f, 2.4f);
}

static inline float linearChannelToSRGB (float cl)
{
	if (cl <= 0.0f)
		return 0.0f;
	else if (cl < 0.0031308f)
		return 12.92f*cl;
	else if (cl < 1.0f)
		return 1.055f*deFloatPow(cl, 0.41666f) - 0.055f;
	else
		return 1.0f;
}

//! Convert sRGB to linear colorspace
Vec4 sRGBToLinear (const Vec4& cs)
{
	return Vec4(sRGBChannelToLinear(cs[0]),
				sRGBChannelToLinear(cs[1]),
				sRGBChannelToLinear(cs[2]),
				cs[3]);
}

//! Convert from linear to sRGB colorspace
Vec4 linearToSRGB (const Vec4& cl)
{
	return Vec4(linearChannelToSRGB(cl[0]),
				linearChannelToSRGB(cl[1]),
				linearChannelToSRGB(cl[2]),
				cl[3]);
}

bool isSRGB (TextureFormat format)
{
	return	format.order == TextureFormat::sR	||
			format.order == TextureFormat::sRG	||
			format.order == TextureFormat::sRGB	||
			format.order == TextureFormat::sRGBA;
}

bool isCombinedDepthStencilType (TextureFormat::ChannelType type)
{
	// make sure to update this if type table is updated
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 27);

	return	type == TextureFormat::UNSIGNED_INT_24_8 ||
			type == TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV;
}

//! Get texture channel class for format
TextureChannelClass getTextureChannelClass (TextureFormat::ChannelType channelType)
{
	// make sure this table is updated if format table is updated
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 27);

	switch (channelType)
	{
		case TextureFormat::SNORM_INT8:						return TEXTURECHANNELCLASS_SIGNED_FIXED_POINT;
		case TextureFormat::SNORM_INT16:					return TEXTURECHANNELCLASS_SIGNED_FIXED_POINT;
		case TextureFormat::SNORM_INT32:					return TEXTURECHANNELCLASS_SIGNED_FIXED_POINT;
		case TextureFormat::UNORM_INT8:						return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_INT16:					return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_INT24:					return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_INT32:					return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_SHORT_565:				return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_SHORT_555:				return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_SHORT_4444:				return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_SHORT_5551:				return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_INT_101010:				return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNORM_INT_1010102_REV:			return TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
		case TextureFormat::UNSIGNED_INT_1010102_REV:		return TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:	return TEXTURECHANNELCLASS_FLOATING_POINT;
		case TextureFormat::UNSIGNED_INT_999_E5_REV:		return TEXTURECHANNELCLASS_FLOATING_POINT;
		case TextureFormat::UNSIGNED_INT_24_8:				return TEXTURECHANNELCLASS_LAST;					//!< packed unorm24-uint8
		case TextureFormat::SIGNED_INT8:					return TEXTURECHANNELCLASS_SIGNED_INTEGER;
		case TextureFormat::SIGNED_INT16:					return TEXTURECHANNELCLASS_SIGNED_INTEGER;
		case TextureFormat::SIGNED_INT32:					return TEXTURECHANNELCLASS_SIGNED_INTEGER;
		case TextureFormat::UNSIGNED_INT8:					return TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
		case TextureFormat::UNSIGNED_INT16:					return TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
		case TextureFormat::UNSIGNED_INT24:					return TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
		case TextureFormat::UNSIGNED_INT32:					return TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
		case TextureFormat::HALF_FLOAT:						return TEXTURECHANNELCLASS_FLOATING_POINT;
		case TextureFormat::FLOAT:							return TEXTURECHANNELCLASS_FLOATING_POINT;
		case TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV:	return TEXTURECHANNELCLASS_LAST;					//!< packed float32-pad24-uint8
		default:											return TEXTURECHANNELCLASS_LAST;
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Get access to subregion of pixel buffer
 * \param access	Parent access object
 * \param x			X offset
 * \param y			Y offset
 * \param z			Z offset
 * \param width		Width
 * \param height	Height
 * \param depth		Depth
 * \return Access object that targets given subregion of parent access object
 *//*--------------------------------------------------------------------*/
ConstPixelBufferAccess getSubregion (const ConstPixelBufferAccess& access, int x, int y, int z, int width, int height, int depth)
{
	DE_ASSERT(de::inBounds(x, 0, access.getWidth()));
	DE_ASSERT(de::inRange(x+width, x+1, access.getWidth()));

	DE_ASSERT(de::inBounds(y, 0, access.getHeight()));
	DE_ASSERT(de::inRange(y+height, y+1, access.getHeight()));

	DE_ASSERT(de::inBounds(z, 0, access.getDepth()));
	DE_ASSERT(de::inRange(z+depth, z+1, access.getDepth()));

	return ConstPixelBufferAccess(access.getFormat(), tcu::IVec3(width, height, depth), access.getPitch(),
								  (const deUint8*)access.getDataPtr() + access.getPixelPitch()*x + access.getRowPitch()*y + access.getSlicePitch()*z);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get access to subregion of pixel buffer
 * \param access	Parent access object
 * \param x			X offset
 * \param y			Y offset
 * \param z			Z offset
 * \param width		Width
 * \param height	Height
 * \param depth		Depth
 * \return Access object that targets given subregion of parent access object
 *//*--------------------------------------------------------------------*/
PixelBufferAccess getSubregion (const PixelBufferAccess& access, int x, int y, int z, int width, int height, int depth)
{
	DE_ASSERT(de::inBounds(x, 0, access.getWidth()));
	DE_ASSERT(de::inRange(x+width, x+1, access.getWidth()));

	DE_ASSERT(de::inBounds(y, 0, access.getHeight()));
	DE_ASSERT(de::inRange(y+height, y+1, access.getHeight()));

	DE_ASSERT(de::inBounds(z, 0, access.getDepth()));
	DE_ASSERT(de::inRange(z+depth, z+1, access.getDepth()));

	return PixelBufferAccess(access.getFormat(), tcu::IVec3(width, height, depth), access.getPitch(),
							 (deUint8*)access.getDataPtr() + access.getPixelPitch()*x + access.getRowPitch()*y + access.getSlicePitch()*z);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get access to subregion of pixel buffer
 * \param access	Parent access object
 * \param x			X offset
 * \param y			Y offset
 * \param width		Width
 * \param height	Height
 * \return Access object that targets given subregion of parent access object
 *//*--------------------------------------------------------------------*/
PixelBufferAccess getSubregion (const PixelBufferAccess& access, int x, int y, int width, int height)
{
	return getSubregion(access, x, y, 0, width, height, 1);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get access to subregion of pixel buffer
 * \param access	Parent access object
 * \param x			X offset
 * \param y			Y offset
 * \param width		Width
 * \param height	Height
 * \return Access object that targets given subregion of parent access object
 *//*--------------------------------------------------------------------*/
ConstPixelBufferAccess getSubregion (const ConstPixelBufferAccess& access, int x, int y, int width, int height)
{
	return getSubregion(access, x, y, 0, width, height, 1);
}

/*--------------------------------------------------------------------*//*!
 * \brief Flip rows in Y direction
 * \param access Access object
 * \return Modified access object where Y coordinates are reversed
 *//*--------------------------------------------------------------------*/
PixelBufferAccess flipYAccess (const PixelBufferAccess& access)
{
	const int			rowPitch		= access.getRowPitch();
	const int			offsetToLast	= rowPitch*(access.getHeight()-1);
	const tcu::IVec3	pitch			(access.getPixelPitch(), -rowPitch, access.getSlicePitch());

	return PixelBufferAccess(access.getFormat(), access.getSize(), pitch, (deUint8*)access.getDataPtr() + offsetToLast);
}

/*--------------------------------------------------------------------*//*!
 * \brief Flip rows in Y direction
 * \param access Access object
 * \return Modified access object where Y coordinates are reversed
 *//*--------------------------------------------------------------------*/
ConstPixelBufferAccess flipYAccess (const ConstPixelBufferAccess& access)
{
	const int			rowPitch		= access.getRowPitch();
	const int			offsetToLast	= rowPitch*(access.getHeight()-1);
	const tcu::IVec3	pitch			(access.getPixelPitch(), -rowPitch, access.getSlicePitch());

	return ConstPixelBufferAccess(access.getFormat(), access.getSize(), pitch, (deUint8*)access.getDataPtr() + offsetToLast);
}

static Vec2 getChannelValueRange (TextureFormat::ChannelType channelType)
{
	// make sure this table is updated if format table is updated
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 27);

	float cMin = 0.0f;
	float cMax = 0.0f;

	switch (channelType)
	{
		// Signed normalized formats.
		case TextureFormat::SNORM_INT8:
		case TextureFormat::SNORM_INT16:
		case TextureFormat::SNORM_INT32:					cMin = -1.0f;			cMax = 1.0f;			break;

		// Unsigned normalized formats.
		case TextureFormat::UNORM_INT8:
		case TextureFormat::UNORM_INT16:
		case TextureFormat::UNORM_INT24:
		case TextureFormat::UNORM_INT32:
		case TextureFormat::UNORM_SHORT_565:
		case TextureFormat::UNORM_SHORT_4444:
		case TextureFormat::UNORM_INT_101010:
		case TextureFormat::UNORM_INT_1010102_REV:			cMin = 0.0f;			cMax = 1.0f;			break;

		// Misc formats.
		case TextureFormat::SIGNED_INT8:					cMin = -128.0f;			cMax = 127.0f;			break;
		case TextureFormat::SIGNED_INT16:					cMin = -32768.0f;		cMax = 32767.0f;		break;
		case TextureFormat::SIGNED_INT32:					cMin = -2147483648.0f;	cMax = 2147483647.0f;	break;
		case TextureFormat::UNSIGNED_INT8:					cMin = 0.0f;			cMax = 255.0f;			break;
		case TextureFormat::UNSIGNED_INT16:					cMin = 0.0f;			cMax = 65535.0f;		break;
		case TextureFormat::UNSIGNED_INT24:					cMin = 0.0f;			cMax = 16777215.0f;		break;
		case TextureFormat::UNSIGNED_INT32:					cMin = 0.0f;			cMax = 4294967295.f;	break;
		case TextureFormat::HALF_FLOAT:						cMin = -1e3f;			cMax = 1e3f;			break;
		case TextureFormat::FLOAT:							cMin = -1e5f;			cMax = 1e5f;			break;
		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:	cMin = 0.0f;			cMax = 1e4f;			break;
		case TextureFormat::UNSIGNED_INT_999_E5_REV:		cMin = 0.0f;			cMax = 1e5f;			break;

		default:
			DE_ASSERT(false);
	}

	return Vec2(cMin, cMax);
}

/*--------------------------------------------------------------------*//*!
 * \brief Get standard parameters for testing texture format
 *
 * Returns TextureFormatInfo that describes good parameters for exercising
 * given TextureFormat. Parameters include value ranges per channel and
 * suitable lookup scaling and bias in order to reduce result back to
 * 0..1 range.
 *//*--------------------------------------------------------------------*/
TextureFormatInfo getTextureFormatInfo (const TextureFormat& format)
{
	// Special cases.
	if (format == TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT_1010102_REV))
		return TextureFormatInfo(Vec4(	    0.0f,		    0.0f,		    0.0f,		 0.0f),
								 Vec4(	 1023.0f,		 1023.0f,		 1023.0f,		 3.0f),
								 Vec4(1.0f/1023.f,	1.0f/1023.0f,	1.0f/1023.0f,	1.0f/3.0f),
								 Vec4(	    0.0f,		    0.0f,		    0.0f,		 0.0f));
	else if (format.order == TextureFormat::D || format.order == TextureFormat::DS)
		return TextureFormatInfo(Vec4(0.0f,	0.0f,	0.0f,	0.0f),
								 Vec4(1.0f,	1.0f,	1.0f,	0.0f),
								 Vec4(1.0f,	1.0f,	1.0f,	1.0f),
								 Vec4(0.0f,	0.0f,	0.0f,	0.0f)); // Depth / stencil formats.
	else if (format == TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_SHORT_5551))
		return TextureFormatInfo(Vec4(0.0f, 0.0f, 0.0f, 0.5f),
								 Vec4(1.0f, 1.0f, 1.0f, 1.5f),
								 Vec4(1.0f, 1.0f, 1.0f, 1.0f),
								 Vec4(0.0f, 0.0f, 0.0f, 0.0f));

	const Vec2						cRange		= getChannelValueRange(format.type);
	const TextureSwizzle::Channel*	map			= getChannelReadSwizzle(format.order).components;
	const BVec4						chnMask		= BVec4(deInRange32(map[0], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[1], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[2], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[3], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE);
	const float						scale		= 1.0f / (cRange[1] - cRange[0]);
	const float						bias		= -cRange[0] * scale;

	return TextureFormatInfo(select(cRange[0],	0.0f, chnMask),
							 select(cRange[1],	0.0f, chnMask),
							 select(scale,		1.0f, chnMask),
							 select(bias,		0.0f, chnMask));
}

static IVec4 getChannelBitDepth (TextureFormat::ChannelType channelType)
{
	// make sure this table is updated if format table is updated
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 27);

	switch (channelType)
	{
		case TextureFormat::SNORM_INT8:						return IVec4(8);
		case TextureFormat::SNORM_INT16:					return IVec4(16);
		case TextureFormat::SNORM_INT32:					return IVec4(32);
		case TextureFormat::UNORM_INT8:						return IVec4(8);
		case TextureFormat::UNORM_INT16:					return IVec4(16);
		case TextureFormat::UNORM_INT24:					return IVec4(24);
		case TextureFormat::UNORM_INT32:					return IVec4(32);
		case TextureFormat::UNORM_SHORT_565:				return IVec4(5,6,5,0);
		case TextureFormat::UNORM_SHORT_4444:				return IVec4(4);
		case TextureFormat::UNORM_SHORT_555:				return IVec4(5,5,5,0);
		case TextureFormat::UNORM_SHORT_5551:				return IVec4(5,5,5,1);
		case TextureFormat::UNORM_INT_101010:				return IVec4(10,10,10,0);
		case TextureFormat::UNORM_INT_1010102_REV:			return IVec4(10,10,10,2);
		case TextureFormat::SIGNED_INT8:					return IVec4(8);
		case TextureFormat::SIGNED_INT16:					return IVec4(16);
		case TextureFormat::SIGNED_INT32:					return IVec4(32);
		case TextureFormat::UNSIGNED_INT8:					return IVec4(8);
		case TextureFormat::UNSIGNED_INT16:					return IVec4(16);
		case TextureFormat::UNSIGNED_INT24:					return IVec4(24);
		case TextureFormat::UNSIGNED_INT32:					return IVec4(32);
		case TextureFormat::UNSIGNED_INT_1010102_REV:		return IVec4(10,10,10,2);
		case TextureFormat::UNSIGNED_INT_24_8:				return IVec4(24,8,0,0);
		case TextureFormat::HALF_FLOAT:						return IVec4(16);
		case TextureFormat::FLOAT:							return IVec4(32);
		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:	return IVec4(11,11,10,0);
		case TextureFormat::UNSIGNED_INT_999_E5_REV:		return IVec4(9,9,9,0);
		case TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV:	return IVec4(32,8,0,0);
		default:
			DE_ASSERT(false);
			return IVec4(0);
	}
}

IVec4 getTextureFormatBitDepth (const TextureFormat& format)
{
	const IVec4						chnBits		= getChannelBitDepth(format.type);
	const TextureSwizzle::Channel*	map			= getChannelReadSwizzle(format.order).components;
	const BVec4						chnMask		= BVec4(deInRange32(map[0], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[1], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[2], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[3], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE);
	const IVec4						chnSwz		= IVec4((chnMask[0]) ? ((int)map[0]) : (0),
														(chnMask[1]) ? ((int)map[1]) : (0),
														(chnMask[2]) ? ((int)map[2]) : (0),
														(chnMask[3]) ? ((int)map[3]) : (0));

	return select(chnBits.swizzle(chnSwz.x(), chnSwz.y(), chnSwz.z(), chnSwz.w()), IVec4(0), chnMask);
}

static IVec4 getChannelMantissaBitDepth (TextureFormat::ChannelType channelType)
{
	// make sure this table is updated if format table is updated
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 27);

	switch (channelType)
	{
		case TextureFormat::SNORM_INT8:
		case TextureFormat::SNORM_INT16:
		case TextureFormat::SNORM_INT32:
		case TextureFormat::UNORM_INT8:
		case TextureFormat::UNORM_INT16:
		case TextureFormat::UNORM_INT24:
		case TextureFormat::UNORM_INT32:
		case TextureFormat::UNORM_SHORT_565:
		case TextureFormat::UNORM_SHORT_4444:
		case TextureFormat::UNORM_SHORT_555:
		case TextureFormat::UNORM_SHORT_5551:
		case TextureFormat::UNORM_INT_101010:
		case TextureFormat::UNORM_INT_1010102_REV:
		case TextureFormat::SIGNED_INT8:
		case TextureFormat::SIGNED_INT16:
		case TextureFormat::SIGNED_INT32:
		case TextureFormat::UNSIGNED_INT8:
		case TextureFormat::UNSIGNED_INT16:
		case TextureFormat::UNSIGNED_INT24:
		case TextureFormat::UNSIGNED_INT32:
		case TextureFormat::UNSIGNED_INT_1010102_REV:
		case TextureFormat::UNSIGNED_INT_24_8:
		case TextureFormat::UNSIGNED_INT_999_E5_REV:
			return getChannelBitDepth(channelType);

		case TextureFormat::HALF_FLOAT:						return IVec4(10);
		case TextureFormat::FLOAT:							return IVec4(23);
		case TextureFormat::UNSIGNED_INT_11F_11F_10F_REV:	return IVec4(6,6,5,0);
		case TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV:	return IVec4(23,8,0,0);
		default:
			DE_ASSERT(false);
			return IVec4(0);
	}
}

IVec4 getTextureFormatMantissaBitDepth (const TextureFormat& format)
{
	const IVec4						chnBits		= getChannelMantissaBitDepth(format.type);
	const TextureSwizzle::Channel*	map			= getChannelReadSwizzle(format.order).components;
	const BVec4						chnMask		= BVec4(deInRange32(map[0], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[1], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[2], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
														deInRange32(map[3], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE);
	const IVec4						chnSwz		= IVec4((chnMask[0]) ? ((int)map[0]) : (0),
														(chnMask[1]) ? ((int)map[1]) : (0),
														(chnMask[2]) ? ((int)map[2]) : (0),
														(chnMask[3]) ? ((int)map[3]) : (0));

	return select(chnBits.swizzle(chnSwz.x(), chnSwz.y(), chnSwz.z(), chnSwz.w()), IVec4(0), chnMask);
}

BVec4 getTextureFormatChannelMask (const TextureFormat& format)
{
	const TextureSwizzle::Channel* const map = getChannelReadSwizzle(format.order).components;
	return BVec4(deInRange32(map[0], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
				 deInRange32(map[1], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
				 deInRange32(map[2], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE,
				 deInRange32(map[3], TextureSwizzle::CHANNEL_0, TextureSwizzle::CHANNEL_3) == DE_TRUE);
}

static inline float linearInterpolate (float t, float minVal, float maxVal)
{
	return minVal + (maxVal - minVal) * t;
}

static inline Vec4 linearInterpolate (float t, const Vec4& a, const Vec4& b)
{
	return a + (b - a) * t;
}

enum
{
	CLEAR_OPTIMIZE_THRESHOLD		= 128,
	CLEAR_OPTIMIZE_MAX_PIXEL_SIZE	= 8
};

inline void fillRow (const PixelBufferAccess& dst, int y, int z, int pixelSize, const deUint8* pixel)
{
	DE_ASSERT(dst.getPixelPitch() == pixelSize); // only tightly packed

	deUint8*	dstPtr	= (deUint8*)dst.getPixelPtr(0, y, z);
	int			width	= dst.getWidth();

	if (pixelSize == 8 && deIsAlignedPtr(dstPtr, pixelSize))
	{
		deUint64 val;
		memcpy(&val, pixel, sizeof(val));

		for (int i = 0; i < width; i++)
			((deUint64*)dstPtr)[i] = val;
	}
	else if (pixelSize == 4 && deIsAlignedPtr(dstPtr, pixelSize))
	{
		deUint32 val;
		memcpy(&val, pixel, sizeof(val));

		for (int i = 0; i < width; i++)
			((deUint32*)dstPtr)[i] = val;
	}
	else
	{
		for (int i = 0; i < width; i++)
			for (int j = 0; j < pixelSize; j++)
				dstPtr[i*pixelSize+j] = pixel[j];
	}
}

void clear (const PixelBufferAccess& access, const Vec4& color)
{
	const int	pixelSize				= access.getFormat().getPixelSize();
	const int	pixelPitch				= access.getPixelPitch();
	const bool	rowPixelsTightlyPacked	= (pixelSize == pixelPitch);

	if (access.getWidth()*access.getHeight()*access.getDepth() >= CLEAR_OPTIMIZE_THRESHOLD &&
		pixelSize < CLEAR_OPTIMIZE_MAX_PIXEL_SIZE && rowPixelsTightlyPacked)
	{
		// Convert to destination format.
		union
		{
			deUint8		u8[CLEAR_OPTIMIZE_MAX_PIXEL_SIZE];
			deUint64	u64; // Forces 64-bit alignment.
		} pixel;
		DE_STATIC_ASSERT(sizeof(pixel) == CLEAR_OPTIMIZE_MAX_PIXEL_SIZE);
		PixelBufferAccess(access.getFormat(), 1, 1, 1, 0, 0, &pixel.u8[0]).setPixel(color, 0, 0);

		for (int z = 0; z < access.getDepth(); z++)
			for (int y = 0; y < access.getHeight(); y++)
				fillRow(access, y, z, pixelSize, &pixel.u8[0]);
	}
	else
	{
		for (int z = 0; z < access.getDepth(); z++)
			for (int y = 0; y < access.getHeight(); y++)
				for (int x = 0; x < access.getWidth(); x++)
					access.setPixel(color, x, y, z);
	}
}

void clear (const PixelBufferAccess& access, const IVec4& color)
{
	const int	pixelSize				= access.getFormat().getPixelSize();
	const int	pixelPitch				= access.getPixelPitch();
	const bool	rowPixelsTightlyPacked	= (pixelSize == pixelPitch);

	if (access.getWidth()*access.getHeight()*access.getDepth() >= CLEAR_OPTIMIZE_THRESHOLD &&
		pixelSize < CLEAR_OPTIMIZE_MAX_PIXEL_SIZE && rowPixelsTightlyPacked)
	{
		// Convert to destination format.
		union
		{
			deUint8		u8[CLEAR_OPTIMIZE_MAX_PIXEL_SIZE];
			deUint64	u64; // Forces 64-bit alignment.
		} pixel;
		DE_STATIC_ASSERT(sizeof(pixel) == CLEAR_OPTIMIZE_MAX_PIXEL_SIZE);
		PixelBufferAccess(access.getFormat(), 1, 1, 1, 0, 0, &pixel.u8[0]).setPixel(color, 0, 0);

		for (int z = 0; z < access.getDepth(); z++)
			for (int y = 0; y < access.getHeight(); y++)
				fillRow(access, y, z, pixelSize, &pixel.u8[0]);
	}
	else
	{
		for (int z = 0; z < access.getDepth(); z++)
			for (int y = 0; y < access.getHeight(); y++)
				for (int x = 0; x < access.getWidth(); x++)
					access.setPixel(color, x, y, z);
	}
}

void clear (const PixelBufferAccess& access, const UVec4& color)
{
	clear(access, color.cast<deInt32>());
}

void clearDepth (const PixelBufferAccess& access, float depth)
{
	DE_ASSERT(access.getFormat().order == TextureFormat::DS || access.getFormat().order == TextureFormat::D);

	clear(getEffectiveDepthStencilAccess(access, Sampler::MODE_DEPTH), tcu::Vec4(depth, 0.0f, 0.0f, 0.0f));
}

void clearStencil (const PixelBufferAccess& access, int stencil)
{
	DE_ASSERT(access.getFormat().order == TextureFormat::DS || access.getFormat().order == TextureFormat::S);

	clear(getEffectiveDepthStencilAccess(access, Sampler::MODE_STENCIL), tcu::UVec4(stencil, 0u, 0u, 0u));
}

static void fillWithComponentGradients1D (const PixelBufferAccess& access, const Vec4& minVal, const Vec4& maxVal)
{
	DE_ASSERT(access.getHeight() == 1);
	for (int x = 0; x < access.getWidth(); x++)
	{
		float s	= ((float)x + 0.5f) / (float)access.getWidth();

		float r	= linearInterpolate(s, minVal.x(), maxVal.x());
		float g = linearInterpolate(s, minVal.y(), maxVal.y());
		float b = linearInterpolate(s, minVal.z(), maxVal.z());
		float a = linearInterpolate(s, minVal.w(), maxVal.w());

		access.setPixel(tcu::Vec4(r, g, b, a), x, 0);
	}
}

static void fillWithComponentGradients2D (const PixelBufferAccess& access, const Vec4& minVal, const Vec4& maxVal)
{
	for (int y = 0; y < access.getHeight(); y++)
	{
		for (int x = 0; x < access.getWidth(); x++)
		{
			float s	= ((float)x + 0.5f) / (float)access.getWidth();
			float t	= ((float)y + 0.5f) / (float)access.getHeight();

			float r	= linearInterpolate((      s  +       t) *0.5f, minVal.x(), maxVal.x());
			float g = linearInterpolate((      s  + (1.0f-t))*0.5f, minVal.y(), maxVal.y());
			float b = linearInterpolate(((1.0f-s) +       t) *0.5f, minVal.z(), maxVal.z());
			float a = linearInterpolate(((1.0f-s) + (1.0f-t))*0.5f, minVal.w(), maxVal.w());

			access.setPixel(tcu::Vec4(r, g, b, a), x, y);
		}
	}
}

static void fillWithComponentGradients3D (const PixelBufferAccess& dst, const Vec4& minVal, const Vec4& maxVal)
{
	for (int z = 0; z < dst.getDepth(); z++)
	{
		for (int y = 0; y < dst.getHeight(); y++)
		{
			for (int x = 0; x < dst.getWidth(); x++)
			{
				float s = ((float)x + 0.5f) / (float)dst.getWidth();
				float t = ((float)y + 0.5f) / (float)dst.getHeight();
				float p = ((float)z + 0.5f) / (float)dst.getDepth();

				float r = linearInterpolate(s,						minVal.x(), maxVal.x());
				float g = linearInterpolate(t,						minVal.y(), maxVal.y());
				float b = linearInterpolate(p,						minVal.z(), maxVal.z());
				float a = linearInterpolate(1.0f - (s+t+p)/3.0f,	minVal.w(), maxVal.w());

				dst.setPixel(tcu::Vec4(r, g, b, a), x, y, z);
			}
		}
	}
}

void fillWithComponentGradients (const PixelBufferAccess& access, const Vec4& minVal, const Vec4& maxVal)
{
	if (access.getHeight() == 1 && access.getDepth() == 1)
		fillWithComponentGradients1D(access, minVal, maxVal);
	else if (access.getDepth() == 1)
		fillWithComponentGradients2D(access, minVal, maxVal);
	else
		fillWithComponentGradients3D(access, minVal, maxVal);
}

void fillWithGrid1D (const PixelBufferAccess& access, int cellSize, const Vec4& colorA, const Vec4& colorB)
{
	for (int x = 0; x < access.getWidth(); x++)
	{
		int mx = (x / cellSize) % 2;

		if (mx)
			access.setPixel(colorB, x, 0);
		else
			access.setPixel(colorA, x, 0);
	}
}

void fillWithGrid2D (const PixelBufferAccess& access, int cellSize, const Vec4& colorA, const Vec4& colorB)
{
	for (int y = 0; y < access.getHeight(); y++)
	{
		for (int x = 0; x < access.getWidth(); x++)
		{
			int mx = (x / cellSize) % 2;
			int my = (y / cellSize) % 2;

			if (mx ^ my)
				access.setPixel(colorB, x, y);
			else
				access.setPixel(colorA, x, y);
		}
	}
}

void fillWithGrid3D (const PixelBufferAccess& access, int cellSize, const Vec4& colorA, const Vec4& colorB)
{
	for (int z = 0; z < access.getDepth(); z++)
	{
		for (int y = 0; y < access.getHeight(); y++)
		{
			for (int x = 0; x < access.getWidth(); x++)
			{
				int mx = (x / cellSize) % 2;
				int my = (y / cellSize) % 2;
				int mz = (z / cellSize) % 2;

				if (mx ^ my ^ mz)
					access.setPixel(colorB, x, y, z);
				else
					access.setPixel(colorA, x, y, z);
			}
		}
	}
}

void fillWithGrid (const PixelBufferAccess& access, int cellSize, const Vec4& colorA, const Vec4& colorB)
{
	if (access.getHeight() == 1 && access.getDepth() == 1)
		fillWithGrid1D(access, cellSize, colorA, colorB);
	else if (access.getDepth() == 1)
		fillWithGrid2D(access, cellSize, colorA, colorB);
	else
		fillWithGrid3D(access, cellSize, colorA, colorB);
}

void fillWithRepeatableGradient (const PixelBufferAccess& access, const Vec4& colorA, const Vec4& colorB)
{
	for (int y = 0; y < access.getHeight(); y++)
	{
		for (int x = 0; x < access.getWidth(); x++)
		{
			float s = ((float)x + 0.5f) / (float)access.getWidth();
			float t = ((float)y + 0.5f) / (float)access.getHeight();

			float a = s > 0.5f ? (2.0f - 2.0f*s) : 2.0f*s;
			float b = t > 0.5f ? (2.0f - 2.0f*t) : 2.0f*t;

			float p = deFloatClamp(deFloatSqrt(a*a + b*b), 0.0f, 1.0f);
			access.setPixel(linearInterpolate(p, colorA, colorB), x, y);
		}
	}
}

void fillWithRGBAQuads (const PixelBufferAccess& dst)
{
	TCU_CHECK_INTERNAL(dst.getDepth() == 1);
	int width	= dst.getWidth();
	int height	= dst.getHeight();
	int	left	= width/2;
	int top		= height/2;

	clear(getSubregion(dst, 0,		0,		0, left,		top,		1),	Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	clear(getSubregion(dst, left,	0,		0, width-left,	top,		1),	Vec4(0.0f, 1.0f, 0.0f, 1.0f));
	clear(getSubregion(dst, 0,		top,	0, left,		height-top,	1), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	clear(getSubregion(dst, left,	top,	0, width-left,	height-top, 1), Vec4(0.5f, 0.5f, 0.5f, 1.0f));
}

// \todo [2012-11-13 pyry] There is much better metaballs code in CL SIR value generators.
void fillWithMetaballs (const PixelBufferAccess& dst, int numBalls, deUint32 seed)
{
	TCU_CHECK_INTERNAL(dst.getDepth() == 1);
	std::vector<Vec2>	points(numBalls);
	de::Random			rnd(seed);

	for (int i = 0; i < numBalls; i++)
	{
		float x = rnd.getFloat();
		float y = rnd.getFloat();
		points[i] = (Vec2(x, y));
	}

	for (int y = 0; y < dst.getHeight(); y++)
	for (int x = 0; x < dst.getWidth(); x++)
	{
		Vec2 p((float)x/(float)dst.getWidth(), (float)y/(float)dst.getHeight());

		float sum = 0.0f;
		for (std::vector<Vec2>::const_iterator i = points.begin(); i != points.end(); i++)
		{
			Vec2	d = p - *i;
			float	f = 0.01f / (d.x()*d.x() + d.y()*d.y());

			sum += f;
		}

		dst.setPixel(Vec4(sum), x, y);
	}
}

void copy (const PixelBufferAccess& dst, const ConstPixelBufferAccess& src)
{
	DE_ASSERT(src.getSize() == dst.getSize());

	const int	width				= dst.getWidth();
	const int	height				= dst.getHeight();
	const int	depth				= dst.getDepth();

	const int	srcPixelSize		= src.getFormat().getPixelSize();
	const int	dstPixelSize		= dst.getFormat().getPixelSize();
	const int	srcPixelPitch		= src.getPixelPitch();
	const int	dstPixelPitch		= dst.getPixelPitch();
	const bool	srcTightlyPacked	= (srcPixelSize == srcPixelPitch);
	const bool	dstTightlyPacked	= (dstPixelSize == dstPixelPitch);

	if (src.getFormat() == dst.getFormat() && srcTightlyPacked && dstTightlyPacked)
	{
		// Fast-path for matching formats.
		for (int z = 0; z < depth; z++)
		for (int y = 0; y < height; y++)
			deMemcpy(dst.getPixelPtr(0, y, z), src.getPixelPtr(0, y, z), srcPixelSize*width);
	}
	else if (src.getFormat() == dst.getFormat())
	{
		// Bit-exact copy for matching formats.
		for (int z = 0; z < depth; z++)
		for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			deMemcpy(dst.getPixelPtr(x, y, z), src.getPixelPtr(x, y, z), srcPixelSize);
	}
	else
	{
		TextureChannelClass		srcClass	= getTextureChannelClass(src.getFormat().type);
		TextureChannelClass		dstClass	= getTextureChannelClass(dst.getFormat().type);
		bool					srcIsInt	= srcClass == TEXTURECHANNELCLASS_SIGNED_INTEGER || srcClass == TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
		bool					dstIsInt	= dstClass == TEXTURECHANNELCLASS_SIGNED_INTEGER || dstClass == TEXTURECHANNELCLASS_UNSIGNED_INTEGER;

		if (srcIsInt && dstIsInt)
		{
			for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				dst.setPixel(src.getPixelInt(x, y, z), x, y, z);
		}
		else
		{
			for (int z = 0; z < depth; z++)
			for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				dst.setPixel(src.getPixel(x, y, z), x, y, z);
		}
	}
}

void scale (const PixelBufferAccess& dst, const ConstPixelBufferAccess& src, Sampler::FilterMode filter)
{
	DE_ASSERT(filter == Sampler::NEAREST || filter == Sampler::LINEAR);

	Sampler sampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE,
					filter, filter, 0.0f, false);

	float sX = (float)src.getWidth() / (float)dst.getWidth();
	float sY = (float)src.getHeight() / (float)dst.getHeight();
	float sZ = (float)src.getDepth() / (float)dst.getDepth();

	if (dst.getDepth() == 1 && src.getDepth() == 1)
	{
		for (int y = 0; y < dst.getHeight(); y++)
		for (int x = 0; x < dst.getWidth(); x++)
			dst.setPixel(src.sample2D(sampler, filter, (x+0.5f)*sX, (y+0.5f)*sY, 0), x, y);
	}
	else
	{
		for (int z = 0; z < dst.getDepth(); z++)
		for (int y = 0; y < dst.getHeight(); y++)
		for (int x = 0; x < dst.getWidth(); x++)
			dst.setPixel(src.sample3D(sampler, filter, (x+0.5f)*sX, (y+0.5f)*sY, (z+0.5f)*sZ), x, y, z);
	}
}

void estimatePixelValueRange (const ConstPixelBufferAccess& access, Vec4& minVal, Vec4& maxVal)
{
	const TextureFormat& format = access.getFormat();

	switch (format.type)
	{
		case TextureFormat::UNORM_INT8:
		case TextureFormat::UNORM_INT16:
			// Normalized unsigned formats.
			minVal = Vec4(0.0f);
			maxVal = Vec4(1.0f);
			break;

		case TextureFormat::SNORM_INT8:
		case TextureFormat::SNORM_INT16:
			// Normalized signed formats.
			minVal = Vec4(-1.0f);
			maxVal = Vec4(+1.0f);
			break;

		default:
			// \note Samples every 4/8th pixel.
			minVal = Vec4(std::numeric_limits<float>::max());
			maxVal = Vec4(std::numeric_limits<float>::min());

			for (int z = 0; z < access.getDepth(); z += 2)
			{
				for (int y = 0; y < access.getHeight(); y += 2)
				{
					for (int x = 0; x < access.getWidth(); x += 2)
					{
						Vec4 p = access.getPixel(x, y, z);

						minVal[0] = (deFloatIsNaN(p[0]) ? minVal[0] : de::min(minVal[0], p[0]));
						minVal[1] = (deFloatIsNaN(p[1]) ? minVal[1] : de::min(minVal[1], p[1]));
						minVal[2] = (deFloatIsNaN(p[2]) ? minVal[2] : de::min(minVal[2], p[2]));
						minVal[3] = (deFloatIsNaN(p[3]) ? minVal[3] : de::min(minVal[3], p[3]));

						maxVal[0] = (deFloatIsNaN(p[0]) ? maxVal[0] : de::max(maxVal[0], p[0]));
						maxVal[1] = (deFloatIsNaN(p[1]) ? maxVal[1] : de::max(maxVal[1], p[1]));
						maxVal[2] = (deFloatIsNaN(p[2]) ? maxVal[2] : de::max(maxVal[2], p[2]));
						maxVal[3] = (deFloatIsNaN(p[3]) ? maxVal[3] : de::max(maxVal[3], p[3]));
					}
				}
			}
			break;
	}
}

void computePixelScaleBias (const ConstPixelBufferAccess& access, Vec4& scale, Vec4& bias)
{
	Vec4 minVal, maxVal;
	estimatePixelValueRange(access, minVal, maxVal);

	const float eps = 0.0001f;

	for (int c = 0; c < 4; c++)
	{
		if (maxVal[c] - minVal[c] < eps)
		{
			scale[c]	= (maxVal[c] < eps) ? 1.0f : (1.0f / maxVal[c]);
			bias[c]		= (c == 3) ? (1.0f - maxVal[c]*scale[c]) : (0.0f - minVal[c]*scale[c]);
		}
		else
		{
			scale[c]	= 1.0f / (maxVal[c] - minVal[c]);
			bias[c]		= 0.0f - minVal[c]*scale[c];
		}
	}
}

int getCubeArrayFaceIndex (CubeFace face)
{
	DE_ASSERT((int)face >= 0 && face < CUBEFACE_LAST);

	switch (face)
	{
		case CUBEFACE_POSITIVE_X:	return 0;
		case CUBEFACE_NEGATIVE_X:	return 1;
		case CUBEFACE_POSITIVE_Y:	return 2;
		case CUBEFACE_NEGATIVE_Y:	return 3;
		case CUBEFACE_POSITIVE_Z:	return 4;
		case CUBEFACE_NEGATIVE_Z:	return 5;

		default:
			return -1;
	}
}

deUint32 packRGB999E5 (const tcu::Vec4& color)
{
	const int	mBits	= 9;
	const int	eBits	= 5;
	const int	eBias	= 15;
	const int	eMax	= (1<<eBits)-1;
	const float	maxVal	= (float)(((1<<mBits) - 1) * (1<<(eMax-eBias))) / (float)(1<<mBits);

	float	rc		= deFloatClamp(color[0], 0.0f, maxVal);
	float	gc		= deFloatClamp(color[1], 0.0f, maxVal);
	float	bc		= deFloatClamp(color[2], 0.0f, maxVal);
	float	maxc	= de::max(rc, de::max(gc, bc));
	int		expp	= de::max(-eBias - 1, deFloorFloatToInt32(deFloatLog2(maxc))) + 1 + eBias;
	float	e		= deFloatPow(2.0f, (float)(expp-eBias-mBits));
	int		maxs	= deFloorFloatToInt32(maxc / e + 0.5f);

	deUint32	exps	= maxs == (1<<mBits) ? expp+1 : expp;
	deUint32	rs		= (deUint32)deClamp32(deFloorFloatToInt32(rc / e + 0.5f), 0, (1<<9)-1);
	deUint32	gs		= (deUint32)deClamp32(deFloorFloatToInt32(gc / e + 0.5f), 0, (1<<9)-1);
	deUint32	bs		= (deUint32)deClamp32(deFloorFloatToInt32(bc / e + 0.5f), 0, (1<<9)-1);

	DE_ASSERT((exps & ~((1<<5)-1)) == 0);
	DE_ASSERT((rs & ~((1<<9)-1)) == 0);
	DE_ASSERT((gs & ~((1<<9)-1)) == 0);
	DE_ASSERT((bs & ~((1<<9)-1)) == 0);

	return rs | (gs << 9) | (bs << 18) | (exps << 27);
}

// Sampler utils

static const void* addOffset (const void* ptr, int numBytes)
{
	return (const deUint8*)ptr + numBytes;
}

static void* addOffset (void* ptr, int numBytes)
{
	return (deUint8*)ptr + numBytes;
}

template <typename AccessType>
static AccessType toSamplerAccess (const AccessType& baseAccess, Sampler::DepthStencilMode mode)
{
	// make sure to update this if type table is updated
	DE_STATIC_ASSERT(TextureFormat::CHANNELTYPE_LAST == 27);

	if (!isCombinedDepthStencilType(baseAccess.getFormat().type))
		return baseAccess;
	else
	{
#if (DE_ENDIANNESS == DE_LITTLE_ENDIAN)
		const deUint32 uint32ByteOffsetBits0To8	= 0; //!< least significant byte in the lowest address
		const deUint32 uint32ByteOffset8To32	= 1;
#else
		const deUint32 uint32ByteOffsetBits0To8	= 3; //!< least significant byte in the highest address
		const deUint32 uint32ByteOffset8To32	= 0;
#endif

		// Sampled channel must exist
		DE_ASSERT(baseAccess.getFormat().order == TextureFormat::DS ||
				  (mode == Sampler::MODE_DEPTH && baseAccess.getFormat().order == TextureFormat::D) ||
				  (mode == Sampler::MODE_STENCIL && baseAccess.getFormat().order == TextureFormat::S));

		// combined formats have multiple channel classes, detect on sampler settings
		switch (baseAccess.getFormat().type)
		{
			case TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV:
			{
				if (mode == Sampler::MODE_DEPTH)
				{
					// select the float component
					return AccessType(TextureFormat(TextureFormat::D, TextureFormat::FLOAT),
									  baseAccess.getSize(),
									  baseAccess.getPitch(),
									  baseAccess.getDataPtr());
				}
				else if (mode == Sampler::MODE_STENCIL)
				{
					// select the uint 8 component
					return AccessType(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8),
									  baseAccess.getSize(),
									  baseAccess.getPitch(),
									  addOffset(baseAccess.getDataPtr(), 4 + uint32ByteOffsetBits0To8));
				}
				else
				{
					// unknown sampler mode
					DE_ASSERT(false);
					return AccessType();
				}
			}

			case TextureFormat::UNSIGNED_INT_24_8:
			{
				if (mode == Sampler::MODE_DEPTH)
				{
					// select the unorm24 component
					return AccessType(TextureFormat(TextureFormat::D, TextureFormat::UNORM_INT24),
									  baseAccess.getSize(),
									  baseAccess.getPitch(),
									  addOffset(baseAccess.getDataPtr(), uint32ByteOffset8To32));
				}
				else if (mode == Sampler::MODE_STENCIL)
				{
					// select the uint 8 component
					return AccessType(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8),
									  baseAccess.getSize(),
									  baseAccess.getPitch(),
									  addOffset(baseAccess.getDataPtr(), uint32ByteOffsetBits0To8));
				}
				else
				{
					// unknown sampler mode
					DE_ASSERT(false);
					return AccessType();
				}
			}

			default:
			{
				// unknown combined format
				DE_ASSERT(false);
				return AccessType();
			}
		}
	}
}

PixelBufferAccess getEffectiveDepthStencilAccess (const PixelBufferAccess& baseAccess, Sampler::DepthStencilMode mode)
{
	return toSamplerAccess<PixelBufferAccess>(baseAccess, mode);
}

ConstPixelBufferAccess getEffectiveDepthStencilAccess (const ConstPixelBufferAccess& baseAccess, Sampler::DepthStencilMode mode)
{
	return toSamplerAccess<ConstPixelBufferAccess>(baseAccess, mode);
}

TextureFormat getEffectiveDepthStencilTextureFormat (const TextureFormat& baseFormat, Sampler::DepthStencilMode mode)
{
	return toSamplerAccess(ConstPixelBufferAccess(baseFormat, IVec3(0, 0, 0), DE_NULL), mode).getFormat();
}

} // tcu
