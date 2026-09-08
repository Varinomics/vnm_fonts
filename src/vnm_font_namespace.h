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
// Callers must not construct the marked family name themselves, and should not
// handle font bytes or family overrides either. Ask for a shipped font by id:
//
//     const vnm_fonts::Registered_font sans =
//         vnm_fonts::register_shipped_font(vnm_fonts::Shipped_font::ROBOTO_CONDENSED_REGULAR);
//     if (!sans.is_valid()) {
//         // sans.error says why; there is no unmarked fallback.
//     }
//     label->setFont(QFont(sans.family));
//
// A hand-written copy of the name is the drift this call exists to prevent, and
// a hand-written override table is how a consumer silently merges two families.

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

// The fonts this library carries, one per file in fonts/. The resource path and
// the family override of each are resolved inside the library, so a consumer
// never repeats either.
enum class Shipped_font
{
    ROBOTO_CONDENSED_REGULAR,
    ROBOTO_CONDENSED_LIGHT,
    FONT_AWESOME_4,
    FONT_AWESOME_7_BRANDS,
    FONT_AWESOME_7_FREE_REGULAR,
    FONT_AWESOME_7_FREE_SOLID,
    NOTO_SANS_SYMBOLS_2,
    JULIAMONO,
    ABEEZEE,
    UBUNTU_MONO_BRONT,
    JETBRAINS_MONO,
    FIRA_CODE,
};

// A patched font, or the reason it could not be patched. The bytes differ from
// the input only in the name table and in head.checkSumAdjustment.
struct Marked_font
{
    QByteArray bytes;
    QString    family;
    QString    error;

    bool is_valid() const { return error.isEmpty(); }
};

// A font registered with QFontDatabase, or the reason it could not be.
//
// The family is the one the font's own name table declares, verified to be
// among those the font database reported. A font database may report several
// families for one face - a typographic family alongside a family name, a
// width-stripped variant DirectWrite derives and no record of the file
// contains, or the same name once per name record - so the reported list is not
// a choice to be made by picking one of them.
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
// Fails when the patch fails, when registration is refused, when any family the
// font database reports lacks the mark, or when the family the name table
// declares is not among those reported. There is no fallback to the unmarked
// font: registering that is the defect this call prevents.
Registered_font register_marked_font(
    const QByteArray&      font_bytes,
    const Family_override& overrides = Family_override());

// Makes the fonts built into this library reachable under ":/vnm_fonts/".
// This library is static, so the linker discards the generated resource
// initializer unless something references it; this call is that reference.
// register_shipped_font calls it, so a consumer that only registers fonts needs
// nothing. A consumer reading ":/vnm_fonts/..." itself must call it first.
// Calling it more than once is harmless.
void initialize_resources();

// Registers one of the fonts this library carries, resolving its resource path
// and its family override from the one table that holds them. Registering the
// same font twice in a process returns the family the first call produced
// rather than registering the bytes again.
Registered_font register_shipped_font(Shipped_font font);

}
