#pragma once

// Registers a shipped font under a family name unique to this process.
//
// The files in fonts/ are byte-verbatim upstream releases and declare the
// family names their authors gave them. A font the user has installed can
// declare the same name, and two files claiming one family name merge into a
// single family entry, after which glyph lookup and rasterisation can be served
// from different files. README.md records the measured case.
//
// The fix lives here rather than in the shipped bytes: the font is patched in
// memory on the way into QFontDatabase, so nothing this repository
// redistributes differs from what upstream published.
//
// Callers must not construct the marked family name themselves. Ask for it:
//
//     const vnm_fonts::Registered_font sans =
//         vnm_fonts::register_marked_font(font_bytes);
//     if (!sans.is_valid()) {
//         // sans.error says why; there is no unmarked fallback.
//     }
//     label->setFont(QFont(sans.family));
//
// A hand-written copy of the name is the drift this call exists to prevent.

#include <QByteArray>
#include <QMap>
#include <QString>

namespace vnm_fonts {

// The mark appended to every family name this library registers.
constexpr const char* k_family_mark = " (vnm)";

// Base values substituted before the mark is appended, keyed by name ID. They
// come from the family_override table in THIRD_PARTY/*.toml, and exist for
// fonts whose upstream name table would otherwise merge two faces into one
// family. Font Awesome 7 Free Solid is the only current user: upstream files it
// under the same typographic family as Font Awesome 7 Free Regular.
using Family_override = QMap<quint16, QString>;

// A patched font, or the reason it could not be patched. The bytes differ from
// the input only in the name table and in head.checkSumAdjustment.
struct Marked_font
{
    QByteArray bytes;
    QString    family;
    QString    error;

    bool is_valid() const { return error.isEmpty(); }
};

// A font registered with QFontDatabase, or the reason it could not be. The
// family is what the font database resolved, not what the caller expected.
struct Registered_font
{
    QString family;
    int     font_id = -1;
    QString error;

    bool is_valid() const { return error.isEmpty(); }
};

// Appends k_family_mark to name IDs 1, 4 and 16, and "-VNM" to the PostScript
// name in name ID 6, in every platform, encoding and language record present.
// Style names (2, 17) and the copyright, version, designer, vendor and licence
// records (0, 5, 7 and 9 to 14) are left exactly as upstream wrote them.
Marked_font mark_font_family(
    const QByteArray&      font_bytes,
    const Family_override& overrides = Family_override());

// Marks the font and hands it to QFontDatabase::addApplicationFontFromData.
// Fails when the patch fails, when registration is refused, or when the family
// the font database resolves does not carry the mark. There is no fallback to
// the unmarked font: registering that is the defect this call prevents.
Registered_font register_marked_font(
    const QByteArray&      font_bytes,
    const Family_override& overrides = Family_override());

}
