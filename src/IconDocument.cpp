#include "IconDocument.h"

#include "Palette.h"

#include <File.h>
#include <fs_attr.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <TypeConstants.h>

#include <errno.h>
#include <new>
#include <string.h>
#include <sys/stat.h>
#include <vector>


namespace {

const char* kProjectMagic = "BEOSICON";
const uint32_t kProjectVersion = 1;

// Legacy BeOS icon attributes. The STD_ICON attributes are what Haiku's
// Tracker uses for a file's own icon; the plain L:ICON/M:ICON names are the
// original BeOS R3-R5 storage locations. Both are written so that old and
// new systems see the same bitmap.
const char* kLargeAttribute = "BEOS:L:ICON";
const char* kMiniAttribute = "BEOS:M:ICON";
const char* kLargeStandardAttribute = "BEOS:L:STD_ICON";
const char* kMiniStandardAttribute = "BEOS:M:STD_ICON";

const type_code kLargeIconType = 0x49434f4e;	// 'ICON'
const type_code kMiniIconType = 0x4d49434e;		// 'MICN'


void
StoreUInt32(uint8_t*& out, uint32_t value)
{
	out[0] = (uint8_t)(value & 0xff);
	out[1] = (uint8_t)((value >> 8) & 0xff);
	out[2] = (uint8_t)((value >> 16) & 0xff);
	out[3] = (uint8_t)((value >> 24) & 0xff);
	out += 4;
}


uint32_t
LoadUInt32(const uint8_t*& in)
{
	uint32_t value = (uint32_t)in[0] | ((uint32_t)in[1] << 8)
		| ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
	in += 4;
	return value;
}


status_t
ReadWholeFile(const char* path, uint8_t** data, size_t* size)
{
	*data = NULL;
	*size = 0;

	BFile file(path, B_READ_ONLY);
	status_t result = file.InitCheck();
	if (result != B_OK)
		return result;

	off_t fileSize = 0;
	result = file.GetSize(&fileSize);
	if (result != B_OK)
		return result;
	if (fileSize <= 0 || fileSize > 4 * 1024 * 1024)
		return B_BAD_DATA;

	uint8_t* buffer = new(std::nothrow) uint8_t[fileSize];
	if (buffer == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = file.Read(buffer, fileSize);
	if (bytesRead != (ssize_t)fileSize) {
		delete[] buffer;
		return bytesRead < 0 ? (status_t)bytesRead : B_ERROR;
	}

	*data = buffer;
	*size = (size_t)fileSize;
	return B_OK;
}


status_t
WriteWholeFile(const char* path, const uint8_t* data, size_t size,
	BString& error)
{
	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t result = file.InitCheck();
	if (result != B_OK) {
		error.SetToFormat("Could not create \"%s\": %s", path,
			strerror(result));
		return result;
	}

	ssize_t written = file.Write(data, size);
	if (written != (ssize_t)size) {
		error.SetToFormat("Could not write \"%s\": %s", path,
			strerror(written < 0 ? (status_t)written : B_ERROR));
		return written < 0 ? (status_t)written : B_ERROR;
	}

	return B_OK;
}

} // namespace


IconDocument::IconDocument()
	:
	fKind(kUntitledDocument),
	fDirty(false)
{
	New();
}


IconDocument::~IconDocument()
{
}


void
IconDocument::New()
{
	for (int32 i = 0; i < kIconVariantCount; i++)
		memset(fPixels[i], IconPalette::kTransparentIndex, 1024);

	fPath = "";
	fKind = kUntitledDocument;
	fDirty = false;
	ClearUndo();
}


int32
IconDocument::VariantWidth(int32 variant)
{
	return (variant == kMiniVariant || variant == kMiniSelectedVariant) ? 16 : 32;
}


int32
IconDocument::VariantHeight(int32 variant)
{
	return VariantWidth(variant);
}


int32
IconDocument::VariantBytes(int32 variant)
{
	return VariantWidth(variant) * VariantHeight(variant);
}


const char*
IconDocument::VariantName(int32 variant)
{
	switch (variant) {
		case kLargeVariant:
			return "Large icon (32 x 32)";
		case kMiniVariant:
			return "Mini icon (16 x 16)";
		case kLargeSelectedVariant:
			return "Large selected icon (32 x 32)";
		case kMiniSelectedVariant:
			return "Mini selected icon (16 x 16)";
	}
	return "Icon";
}


const char*
IconDocument::VariantShortName(int32 variant)
{
	switch (variant) {
		case kLargeVariant:
			return "Large";
		case kMiniVariant:
			return "Mini";
		case kLargeSelectedVariant:
			return "Large sel.";
		case kMiniSelectedVariant:
			return "Mini sel.";
	}
	return "Icon";
}


uint8*
IconDocument::Pixels(int32 variant)
{
	if (variant < 0 || variant >= kIconVariantCount)
		return NULL;
	return fPixels[variant];
}


const uint8*
IconDocument::Pixels(int32 variant) const
{
	if (variant < 0 || variant >= kIconVariantCount)
		return NULL;
	return fPixels[variant];
}


BString
IconDocument::FileName() const
{
	if (fPath.Length() == 0)
		return BString("Untitled");

	BPath path(fPath.String());
	const char* leaf = path.Leaf();
	return BString(leaf != NULL ? leaf : fPath.String());
}


status_t
IconDocument::Open(const char* path, BString& error)
{
	error = "";
	if (path == NULL || *path == '\0')
		return B_BAD_VALUE;

	struct stat status;
	if (stat(path, &status) != 0) {
		error.SetToFormat("Cannot open \"%s\": %s", path, strerror(errno));
		return B_ENTRY_NOT_FOUND;
	}

	New();

	// A project file is recognised by its magic before anything else.
	uint8_t* fileData = NULL;
	size_t fileSize = 0;
	bool haveFileData = S_ISREG(status.st_mode)
		&& ReadWholeFile(path, &fileData, &fileSize) == B_OK;

	if (haveFileData && fileSize >= 8
		&& memcmp(fileData, kProjectMagic, 8) == 0) {
		status_t result = _ReadProject(fileData, fileSize, error);
		delete[] fileData;
		if (result == B_OK) {
			fPath = path;
			fKind = kProjectDocument;
			fDirty = false;
			ClearUndo();
		}
		return result;
	}

	// BeOS icons normally live in extended attributes of an ordinary file.
	BString attributeError;
	if (_ReadAttributes(path, attributeError) == B_OK) {
		delete[] fileData;
		fPath = path;
		fKind = kAttributeDocument;
		fDirty = false;
		ClearUndo();
		return B_OK;
	}

	if (haveFileData && (fileSize == 1024 || fileSize == 256)) {
		status_t result = _ReadRaw(fileData, fileSize);
		delete[] fileData;
		if (result == B_OK) {
			fPath = path;
			fKind = fileSize == 1024 ? kRawLargeDocument : kRawMiniDocument;
			fDirty = false;
			ClearUndo();
			return B_OK;
		}
		error = "The raw icon data is invalid.";
		return result;
	}

	delete[] fileData;

	if (attributeError.Length() > 0)
		error = attributeError;
	else
		error.SetToFormat("\"%s\" is not a Bitmap-o-matic project, a raw "
			"BeOS icon (1024 or 256 bytes) or a file with BEOS:L:ICON / "
			"BEOS:M:ICON attributes.", path);
	return B_BAD_DATA;
}


status_t
IconDocument::_ReadProject(const uint8* data, size_t size, BString& error)
{
	const uint8* current = data;
	const uint8* end = data + size;

	if (size < 8 + 4 + 4) {
		error = "The project file is truncated.";
		return B_BAD_DATA;
	}

	current += 8; // magic
	uint32 version = LoadUInt32(current);
	if (version != kProjectVersion) {
		error.SetToFormat("Unsupported project version %lu.",
			(unsigned long)version);
		return B_BAD_DATA;
	}

	uint32 count = LoadUInt32(current);
	if (count == 0 || count > (uint32)kIconVariantCount) {
		error = "The project file contains an invalid icon count.";
		return B_BAD_DATA;
	}

	for (uint32 i = 0; i < count; i++) {
		if ((size_t)(end - current) < 12) {
			error = "The project file is truncated.";
			return B_BAD_DATA;
		}
		uint32 width = LoadUInt32(current);
		uint32 height = LoadUInt32(current);
		uint32 length = LoadUInt32(current);

		if ((size_t)(end - current) < length) {
			error = "The project file is truncated.";
			return B_BAD_DATA;
		}

		if (width != (uint32)VariantWidth(i) || height != width
			|| length != (uint32)VariantBytes(i)) {
			error.SetToFormat("The project file contains an icon of an "
				"unsupported size (%lux%lu).", (unsigned long)width,
				(unsigned long)height);
			return B_BAD_DATA;
		}

		memcpy(fPixels[i], current, length);
		current += length;
	}

	return B_OK;
}


status_t
IconDocument::_ReadRaw(const uint8* data, size_t size)
{
	int32 variant;
	if (size == 1024)
		variant = kLargeVariant;
	else if (size == 256)
		variant = kMiniVariant;
	else
		return B_BAD_DATA;

	memcpy(fPixels[variant], data, size);
	return B_OK;
}


status_t
IconDocument::_ReadAttributes(const char* path, BString& error)
{
	BNode node(path);
	status_t result = node.InitCheck();
	if (result != B_OK) {
		error.SetToFormat("Cannot open \"%s\": %s", path, strerror(result));
		return result;
	}

	bool foundLarge = false;
	bool foundMini = false;
	bool found = false;
	BString detail;

	if (_ReadAttribute(node, kLargeAttribute, kLargeVariant, found,
			detail) == B_OK && found) {
		foundLarge = true;
	}
	if (!foundLarge && _ReadAttribute(node, kLargeStandardAttribute,
			kLargeVariant, found, detail) == B_OK && found) {
		foundLarge = true;
	}

	if (_ReadAttribute(node, kMiniAttribute, kMiniVariant, found,
			detail) == B_OK && found) {
		foundMini = true;
	}
	if (!foundMini && _ReadAttribute(node, kMiniStandardAttribute,
			kMiniVariant, found, detail) == B_OK && found) {
		foundMini = true;
	}

	if (!foundLarge && !foundMini) {
		error.SetToFormat("\"%s\" carries no BEOS:L:ICON or BEOS:M:ICON "
			"bitmap icon attributes.", path);
		return B_ENTRY_NOT_FOUND;
	}

	return B_OK;
}


status_t
IconDocument::_ReadAttribute(BNode& node, const char* name, int32 variant,
	bool& found, BString& error)
{
	found = false;

	attr_info info;
	status_t result = node.GetAttrInfo(name, &info);
	if (result != B_OK)
		return result;

	if (info.size <= 0 || info.size > 64 * 1024) {
		error.SetToFormat("The \"%s\" attribute has an implausible size "
			"(%ld bytes).", name, (long)info.size);
		return B_BAD_DATA;
	}

	uint8_t* data = new(std::nothrow) uint8_t[(size_t)info.size];
	if (data == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = node.ReadAttr(name, info.type, 0, data,
		(size_t)info.size);
	if (bytesRead != (ssize_t)info.size) {
		delete[] data;
		error.SetToFormat("Could not read the \"%s\" attribute.", name);
		return bytesRead < 0 ? (status_t)bytesRead : B_ERROR;
	}

	bool imported = _ImportAttributeData(data, (size_t)info.size, variant,
		fPixels[variant]);
	delete[] data;

	if (!imported) {
		error.SetToFormat("The \"%s\" attribute uses an unsupported icon "
			"format (%ld bytes).", name, (long)info.size);
		return B_BAD_TYPE;
	}

	found = true;
	return B_OK;
}


bool
IconDocument::_ImportAttributeData(const uint8* data, size_t size,
	int32 variant, uint8* destination)
{
	int32 bytes = VariantBytes(variant);

	if (size == (size_t)bytes) {
		// B_CMAP8, the classic BeOS bitmap icon.
		memcpy(destination, data, size);
		return true;
	}

	if (size == (size_t)bytes * 4) {
		// B_RGBA32, used by some later BeOS/Haiku applications.
		_ConvertRGBA32(data, variant, destination);
		return true;
	}

	return false;
}


void
IconDocument::_ConvertRGBA32(const uint8* data, int32 variant,
	uint8* destination)
{
	int32 width = VariantWidth(variant);
	int32 height = VariantHeight(variant);

	for (int32 i = 0; i < width * height; i++) {
		// B_RGBA32 is stored little-endian as B, G, R, A.
		uint8 blue = data[i * 4];
		uint8 green = data[i * 4 + 1];
		uint8 red = data[i * 4 + 2];
		uint8 alpha = data[i * 4 + 3];

		if (alpha < 128)
			destination[i] = IconPalette::kTransparentIndex;
		else
			destination[i] = IconPalette::ClosestIndex(red, green, blue);
	}
}


status_t
IconDocument::Save(BString& error)
{
	error = "";

	if (fPath.Length() == 0)
		return B_NO_INIT;

	status_t result;
	switch (fKind) {
		case kProjectDocument:
			result = SaveProject(fPath.String(), error);
			break;
		case kRawLargeDocument:
			result = SaveRaw(fPath.String(), kLargeVariant, error);
			break;
		case kRawMiniDocument:
			result = SaveRaw(fPath.String(), kMiniVariant, error);
			break;
		case kAttributeDocument:
			result = WriteIconAttributes(fPath.String(), error);
			break;
		default:
			error = "This document has no file name yet.";
			return B_NO_INIT;
	}

	if (result == B_OK)
		fDirty = false;

	return result;
}


status_t
IconDocument::SaveProject(const char* path, BString& error)
{
	error = "";

	const size_t headerSize = 8 + 4 + 4;
	size_t total = headerSize;
	for (int32 i = 0; i < kIconVariantCount; i++)
		total += 12 + VariantBytes(i);

	std::vector<uint8_t> buffer(total);
	uint8_t* current = &buffer[0];

	memcpy(current, kProjectMagic, 8);
	current += 8;
	StoreUInt32(current, kProjectVersion);
	StoreUInt32(current, kIconVariantCount);

	for (int32 i = 0; i < kIconVariantCount; i++) {
		int32 bytes = VariantBytes(i);
		StoreUInt32(current, VariantWidth(i));
		StoreUInt32(current, VariantHeight(i));
		StoreUInt32(current, bytes);
		memcpy(current, fPixels[i], bytes);
		current += bytes;
	}

	status_t result = WriteWholeFile(path, &buffer[0], buffer.size(), error);
	if (result == B_OK) {
		fPath = path;
		fKind = kProjectDocument;
		fDirty = false;
	}
	return result;
}


status_t
IconDocument::SaveRaw(const char* path, int32 variant, BString& error)
{
	error = "";
	if (variant < 0 || variant >= kIconVariantCount)
		return B_BAD_VALUE;

	return WriteWholeFile(path, fPixels[variant], VariantBytes(variant),
		error);
}


status_t
IconDocument::WriteIconAttributes(const char* path, BString& error)
{
	error = "";

	BNode node(path);
	status_t result = node.InitCheck();
	if (result != B_OK) {
		error.SetToFormat("Cannot open \"%s\": %s", path, strerror(result));
		return result;
	}

	struct {
		const char* name;
		int32 variant;
		type_code type;
	} attributes[] = {
		{ kLargeAttribute, kLargeVariant, kLargeIconType },
		{ kMiniAttribute, kMiniVariant, kMiniIconType },
		{ kLargeStandardAttribute, kLargeVariant, kLargeIconType },
		{ kMiniStandardAttribute, kMiniVariant, kMiniIconType }
	};

	for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); i++) {
		int32 bytes = VariantBytes(attributes[i].variant);
		ssize_t written = node.WriteAttr(attributes[i].name,
			attributes[i].type, 0, fPixels[attributes[i].variant], bytes);
		if (written != bytes) {
			error.SetToFormat("Could not write the \"%s\" attribute: %s",
				attributes[i].name,
				strerror(written < 0 ? (status_t)written : B_ERROR));
			return written < 0 ? (status_t)written : B_ERROR;
		}
	}

	node.Sync();

	fPath = path;
	fKind = kAttributeDocument;
	fDirty = false;
	return B_OK;
}


bool
IconDocument::HasIconAttributes(const char* path)
{
	BNode node(path);
	if (node.InitCheck() != B_OK)
		return false;

	attr_info info;
	return node.GetAttrInfo(kLargeAttribute, &info) == B_OK
		|| node.GetAttrInfo(kMiniAttribute, &info) == B_OK
		|| node.GetAttrInfo(kLargeStandardAttribute, &info) == B_OK
		|| node.GetAttrInfo(kMiniStandardAttribute, &info) == B_OK;
}


void
IconDocument::PushUndo(int32 variant)
{
	if (variant < 0 || variant >= kIconVariantCount)
		return;

	UndoState state;
	state.variant = variant;
	memcpy(state.pixels, fPixels[variant], VariantBytes(variant));
	fUndo.push_back(state);

	if (fUndo.size() > 64)
		fUndo.erase(fUndo.begin());

	fRedo.clear();
}


bool
IconDocument::Undo()
{
	if (fUndo.empty())
		return false;

	UndoState state = fUndo.back();
	fUndo.pop_back();

	UndoState current;
	current.variant = state.variant;
	memcpy(current.pixels, fPixels[state.variant],
		VariantBytes(state.variant));
	fRedo.push_back(current);

	memcpy(fPixels[state.variant], state.pixels,
		VariantBytes(state.variant));

	fDirty = true;
	return true;
}


bool
IconDocument::Redo()
{
	if (fRedo.empty())
		return false;

	UndoState state = fRedo.back();
	fRedo.pop_back();

	UndoState current;
	current.variant = state.variant;
	memcpy(current.pixels, fPixels[state.variant],
		VariantBytes(state.variant));
	fUndo.push_back(current);

	memcpy(fPixels[state.variant], state.pixels,
		VariantBytes(state.variant));

	fDirty = true;
	return true;
}


void
IconDocument::ClearUndo()
{
	fUndo.clear();
	fRedo.clear();
}
