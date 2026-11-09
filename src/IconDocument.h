#ifndef ICON_DOCUMENT_H
#define ICON_DOCUMENT_H

#include <String.h>
#include <SupportDefs.h>

#include <vector>

// A legacy BeOS icon set consists of a large (32x32) and a mini (16x16)
// bitmap. Icon-O-Matic also edits the selected-state variants, so all four
// images are kept in one document.
enum IconVariant {
	kLargeVariant = 0,
	kMiniVariant = 1,
	kLargeSelectedVariant = 2,
	kMiniSelectedVariant = 3,
	kIconVariantCount = 4
};

enum DocumentKind {
	kUntitledDocument = 0,
	kProjectDocument,
	kRawLargeDocument,
	kRawMiniDocument,
	kAttributeDocument
};

class IconDocument {
public:
	IconDocument();
	~IconDocument();

	void New();

	static int32 VariantWidth(int32 variant);
	static int32 VariantHeight(int32 variant);
	static int32 VariantBytes(int32 variant);
	static const char* VariantName(int32 variant);
	static const char* VariantShortName(int32 variant);

	uint8* Pixels(int32 variant);
	const uint8* Pixels(int32 variant) const;

	bool IsDirty() const { return fDirty; }
	void SetDirty(bool dirty) { fDirty = dirty; }

	DocumentKind Kind() const { return fKind; }
	const char* Path() const { return fPath.String(); }
	BString FileName() const;

	// Opens a bitmap icon project, a raw icon stream, or a file that carries
	// BEOS:L:ICON / BEOS:M:ICON attributes. The format is detected from the
	// contents, not from the file name.
	status_t Open(const char* path, BString& error);

	// Saves in the format the document was opened with.
	status_t Save(BString& error);

	status_t SaveProject(const char* path, BString& error);
	status_t SaveRaw(const char* path, int32 variant, BString& error);
	status_t WriteIconAttributes(const char* path, BString& error);

	static bool HasIconAttributes(const char* path);

	// Undo operates on a single variant at a time. PushUndo() records the
	// current state of the variant before it is modified.
	void PushUndo(int32 variant);
	bool Undo();
	bool Redo();
	bool CanUndo() const { return !fUndo.empty(); }
	bool CanRedo() const { return !fRedo.empty(); }
	void ClearUndo();

private:
	struct UndoState {
		int32_t variant;
		uint8_t pixels[1024];
	};

	status_t _ReadProject(const uint8* data, size_t size, BString& error);
	status_t _ReadRaw(const uint8* data, size_t size);
	status_t _ReadAttributes(const char* path, BString& error);

	status_t _ReadAttribute(class BNode& node, const char* name,
		int32 variant, bool& found, BString& error);
	static bool _ImportAttributeData(const uint8* data, size_t size,
		int32 variant, uint8* destination);
	static void _ConvertRGBA32(const uint8* data, int32 variant,
		uint8* destination);

	uint8_t fPixels[kIconVariantCount][1024];
	BString fPath;
	DocumentKind fKind;
	bool fDirty;

	std::vector<UndoState> fUndo;
	std::vector<UndoState> fRedo;
};

#endif // ICON_DOCUMENT_H
