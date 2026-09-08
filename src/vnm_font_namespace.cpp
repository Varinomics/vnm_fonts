#include "vnm_font_namespace.h"

#include <QFontDatabase>
#include <QStringConverter>
#include <QStringList>
#include <algorithm>
#include <vector>

namespace vnm_fonts {
namespace {

constexpr quint32 k_sfnt_version_truetype = 0x00010000u;
constexpr quint32 k_sfnt_version_cff      = 0x4F54544Fu; // 'OTTO'

constexpr int k_sfnt_header_size          = 12;
constexpr int k_table_directory_entry_size = 16;
constexpr int k_name_table_header_size    = 6;
constexpr int k_name_record_size          = 12;

// head.checkSumAdjustment sits eight bytes into the table, and the whole-file
// checksum is defined to sum to this magic once the adjustment is correct.
constexpr int    k_head_checksum_adjustment_offset = 8;
constexpr quint32 k_head_checksum_magic            = 0xB1B0AFBAu;

constexpr quint16 k_name_id_family             = 1;
constexpr quint16 k_name_id_full               = 4;
constexpr quint16 k_name_id_postscript         = 6;
constexpr quint16 k_name_id_typographic_family = 16;

constexpr quint16 k_platform_unicode   = 0;
constexpr quint16 k_platform_macintosh = 1;
constexpr quint16 k_platform_windows   = 3;

// name ID 6 must stay printable ASCII with no spaces or parentheses, so the
// mark is spelled as a bare token there.
constexpr const char* k_postscript_mark = "-VNM";

struct Sfnt_table
{
    QByteArray tag;
    quint32    offset = 0;
    quint32    length = 0;
    QByteArray data;
};

struct Name_record
{
    quint16    platform_id = 0;
    quint16    encoding_id = 0;
    quint16    language_id = 0;
    quint16    name_id     = 0;
    QByteArray value;
};

quint16 read_u16(const QByteArray& data, int offset)
{
    const auto high = static_cast<quint8>(data.at(offset));
    const auto low  = static_cast<quint8>(data.at(offset + 1));
    return static_cast<quint16>((high << 8) | low);
}

quint32 read_u32(const QByteArray& data, int offset)
{
    return (static_cast<quint32>(read_u16(data, offset)) << 16) | read_u16(data, offset + 2);
}

void append_u16(QByteArray& data, quint16 value)
{
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
}

void append_u32(QByteArray& data, quint32 value)
{
    append_u16(data, static_cast<quint16>((value >> 16) & 0xFFFF));
    append_u16(data, static_cast<quint16>(value & 0xFFFF));
}

bool is_two_byte_encoding(quint16 platform_id)
{
    return platform_id == k_platform_windows || platform_id == k_platform_unicode;
}

// Macintosh Roman and Latin-1 agree over the printable ASCII range, which is
// all these fonts use in the records this library rewrites. A value outside
// that range would be silently mangled, so decoding refuses it rather than
// guessing.
bool decode_name_value(const Name_record& record, QString* out_value)
{
    if (is_two_byte_encoding(record.platform_id)) {
        if (record.value.size() % 2 != 0) {
            return false;
        }
        auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
        *out_value   = decoder(record.value);
        return !decoder.hasError();
    }
    if (record.platform_id != k_platform_macintosh) {
        return false;
    }
    for (const char byte : record.value) {
        if (static_cast<quint8>(byte) >= 0x80) {
            return false;
        }
    }
    *out_value = QString::fromLatin1(record.value);
    return true;
}

bool encode_name_value(const Name_record& record, const QString& value, QByteArray* out_bytes)
{
    if (is_two_byte_encoding(record.platform_id)) {
        auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
        *out_bytes   = encoder(value);
        return !encoder.hasError();
    }
    for (const QChar character : value) {
        if (character.unicode() >= 0x80) {
            return false;
        }
    }
    *out_bytes = value.toLatin1();
    return true;
}

quint32 table_checksum(const QByteArray& data)
{
    quint32 sum   = 0;
    const int end = data.size();
    for (int offset = 0; offset < end; offset += 4) {
        quint32 word = 0;
        for (int byte = 0; byte < 4; ++byte) {
            const int index = offset + byte;
            const auto value = index < end ? static_cast<quint8>(data.at(index)) : quint8(0);
            word = (word << 8) | value;
        }
        sum += word;
    }
    return sum;
}

QByteArray padded_to_four(const QByteArray& data)
{
    QByteArray padded = data;
    while (padded.size() % 4 != 0) {
        padded.append('\0');
    }
    return padded;
}

bool read_tables(const QByteArray& font_bytes, std::vector<Sfnt_table>* out_tables, QString* out_error)
{
    if (font_bytes.size() < k_sfnt_header_size) {
        *out_error = QStringLiteral("The font is too short to contain an sfnt header.");
        return false;
    }
    const quint32 version = read_u32(font_bytes, 0);
    if (version != k_sfnt_version_truetype && version != k_sfnt_version_cff) {
        *out_error = QStringLiteral("Unsupported sfnt version 0x%1.").arg(version, 8, 16, QChar('0'));
        return false;
    }

    const int table_count = read_u16(font_bytes, 4);
    const int directory_end = k_sfnt_header_size + table_count * k_table_directory_entry_size;
    if (font_bytes.size() < directory_end) {
        *out_error = QStringLiteral("The table directory runs past the end of the font.");
        return false;
    }

    out_tables->reserve(static_cast<size_t>(table_count));
    for (int index = 0; index < table_count; ++index) {
        const int entry = k_sfnt_header_size + index * k_table_directory_entry_size;

        Sfnt_table table;
        table.tag    = font_bytes.mid(entry, 4);
        table.offset = read_u32(font_bytes, entry + 8);
        table.length = read_u32(font_bytes, entry + 12);

        const qint64 end = static_cast<qint64>(table.offset) + table.length;
        if (end > font_bytes.size()) {
            *out_error = QStringLiteral("Table %1 runs past the end of the font.")
                             .arg(QString::fromLatin1(table.tag));
            return false;
        }
        table.data = font_bytes.mid(static_cast<int>(table.offset), static_cast<int>(table.length));
        out_tables->push_back(table);
    }
    return true;
}

bool read_name_records(
    const QByteArray&         name_table,
    std::vector<Name_record>* out_records,
    QString*                  out_error)
{
    if (name_table.size() < k_name_table_header_size) {
        *out_error = QStringLiteral("The name table is too short to contain a header.");
        return false;
    }
    const quint16 format = read_u16(name_table, 0);
    if (format != 0) {
        // Format 1 adds a language-tag array whose indices would have to be
        // carried through the rebuild. No shipped font uses it; a font that
        // does must be handled deliberately rather than silently mangled.
        *out_error = QStringLiteral("Unsupported name table format %1.").arg(format);
        return false;
    }

    const int record_count = read_u16(name_table, 2);
    const int string_base  = read_u16(name_table, 4);
    const int records_end  = k_name_table_header_size + record_count * k_name_record_size;
    if (name_table.size() < records_end) {
        *out_error = QStringLiteral("The name records run past the end of the name table.");
        return false;
    }

    out_records->reserve(static_cast<size_t>(record_count));
    for (int index = 0; index < record_count; ++index) {
        const int entry = k_name_table_header_size + index * k_name_record_size;

        Name_record record;
        record.platform_id  = read_u16(name_table, entry);
        record.encoding_id  = read_u16(name_table, entry + 2);
        record.language_id  = read_u16(name_table, entry + 4);
        record.name_id      = read_u16(name_table, entry + 6);
        const int length    = read_u16(name_table, entry + 8);
        const int offset    = read_u16(name_table, entry + 10);

        const qint64 end = static_cast<qint64>(string_base) + offset + length;
        if (end > name_table.size()) {
            *out_error = QStringLiteral("Name record %1 runs past the end of the name table.")
                             .arg(index);
            return false;
        }
        record.value = name_table.mid(string_base + offset, length);
        out_records->push_back(record);
    }
    return true;
}

QByteArray write_name_table(const std::vector<Name_record>& records)
{
    const int record_count = static_cast<int>(records.size());
    const int string_base  = k_name_table_header_size + record_count * k_name_record_size;

    QByteArray table;
    append_u16(table, 0);
    append_u16(table, static_cast<quint16>(record_count));
    append_u16(table, static_cast<quint16>(string_base));

    QByteArray strings;
    for (const Name_record& record : records) {
        append_u16(table, record.platform_id);
        append_u16(table, record.encoding_id);
        append_u16(table, record.language_id);
        append_u16(table, record.name_id);
        append_u16(table, static_cast<quint16>(record.value.size()));
        append_u16(table, static_cast<quint16>(strings.size()));
        strings.append(record.value);
    }
    table.append(strings);
    return table;
}

QByteArray write_font(const std::vector<Sfnt_table>& tables, quint32 sfnt_version, int* out_head_offset)
{
    const int table_count = static_cast<int>(tables.size());

    // The binary-search fields are derived from the table count, and readers
    // that use them expect the values the specification prescribes.
    quint16 entry_selector = 0;
    while ((1 << (entry_selector + 1)) <= table_count) {
        ++entry_selector;
    }
    const quint16 search_range = static_cast<quint16>((1 << entry_selector) * 16);
    const quint16 range_shift  = static_cast<quint16>(table_count * 16 - search_range);

    QByteArray header;
    append_u32(header, sfnt_version);
    append_u16(header, static_cast<quint16>(table_count));
    append_u16(header, search_range);
    append_u16(header, entry_selector);
    append_u16(header, range_shift);

    QByteArray directory;
    QByteArray body;
    quint32    offset = static_cast<quint32>(k_sfnt_header_size +
                                             table_count * k_table_directory_entry_size);
    *out_head_offset = -1;
    for (const Sfnt_table& table : tables) {
        const QByteArray padded = padded_to_four(table.data);
        if (table.tag == QByteArrayLiteral("head")) {
            *out_head_offset = static_cast<int>(offset);
        }

        directory.append(table.tag);
        append_u32(directory, table_checksum(padded));
        append_u32(directory, offset);
        append_u32(directory, static_cast<quint32>(table.data.size()));

        body.append(padded);
        offset += static_cast<quint32>(padded.size());
    }
    return header + directory + body;
}

// head.checkSumAdjustment must be zero while the table checksums are taken, and
// then set so the whole file sums to the prescribed magic.
void clear_checksum_adjustment(std::vector<Sfnt_table>* tables)
{
    for (Sfnt_table& table : *tables) {
        if (table.tag != QByteArrayLiteral("head")) {
            continue;
        }
        if (table.data.size() < k_head_checksum_adjustment_offset + 4) {
            return;
        }
        for (int byte = 0; byte < 4; ++byte) {
            table.data[k_head_checksum_adjustment_offset + byte] = char(0);
        }
        return;
    }
}

void write_checksum_adjustment(QByteArray* font_bytes, int head_offset)
{
    if (head_offset < 0) {
        return;
    }
    const int field = head_offset + k_head_checksum_adjustment_offset;
    const quint32 adjustment = k_head_checksum_magic - table_checksum(*font_bytes);
    for (int byte = 0; byte < 4; ++byte) {
        (*font_bytes)[field + byte] = static_cast<char>((adjustment >> (24 - 8 * byte)) & 0xFF);
    }
}

QString marked_family_of(const std::vector<Name_record>& records)
{
    // A font database prefers the typographic family where the font declares
    // one, so the marked family this call reports has to prefer it too.
    for (const quint16 name_id : {k_name_id_typographic_family, k_name_id_family}) {
        for (const bool windows_only : {true, false}) {
            for (const Name_record& record : records) {
                if (record.name_id != name_id) {
                    continue;
                }
                if (windows_only && record.platform_id != k_platform_windows) {
                    continue;
                }
                QString value;
                if (decode_name_value(record, &value)) {
                    return value;
                }
            }
        }
    }
    return QString();
}

}

Marked_font mark_font_family(const QByteArray& font_bytes, const Family_override& overrides)
{
    Marked_font marked;

    std::vector<Sfnt_table> tables;
    if (!read_tables(font_bytes, &tables, &marked.error)) {
        return marked;
    }

    const auto name_table = std::find_if(
        tables.begin(),
        tables.end(),
        [](const Sfnt_table& table) { return table.tag == QByteArrayLiteral("name"); });
    if (name_table == tables.end()) {
        marked.error = QStringLiteral("The font has no name table.");
        return marked;
    }

    std::vector<Name_record> records;
    if (!read_name_records(name_table->data, &records, &marked.error)) {
        return marked;
    }

    int rewritten = 0;
    for (Name_record& record : records) {
        const bool is_family_name =
            record.name_id == k_name_id_family ||
            record.name_id == k_name_id_full   ||
            record.name_id == k_name_id_typographic_family;
        if (!is_family_name && record.name_id != k_name_id_postscript) {
            continue;
        }

        QString value;
        if (!decode_name_value(record, &value)) {
            marked.error = QStringLiteral("Name ID %1 on platform %2 is not decodable.")
                               .arg(record.name_id).arg(record.platform_id);
            return marked;
        }

        const QString base    = is_family_name ? overrides.value(record.name_id, value) : value;
        const QString updated = is_family_name
            ? base + QString::fromLatin1(k_family_mark)
            : base + QString::fromLatin1(k_postscript_mark);

        if (!encode_name_value(record, updated, &record.value)) {
            marked.error = QStringLiteral("Name ID %1 cannot be encoded for platform %2.")
                               .arg(record.name_id).arg(record.platform_id);
            return marked;
        }
        ++rewritten;
    }

    if (rewritten == 0) {
        marked.error = QStringLiteral("The font declares no family name to mark.");
        return marked;
    }

    marked.family = marked_family_of(records);
    if (marked.family.isEmpty()) {
        marked.error = QStringLiteral("The marked font has no readable family name.");
        return marked;
    }

    name_table->data = write_name_table(records);
    clear_checksum_adjustment(&tables);

    int head_offset = -1;
    marked.bytes    = write_font(tables, read_u32(font_bytes, 0), &head_offset);
    write_checksum_adjustment(&marked.bytes, head_offset);
    return marked;
}

Registered_font register_marked_font(const QByteArray& font_bytes, const Family_override& overrides)
{
    Registered_font registered;

    const Marked_font marked = mark_font_family(font_bytes, overrides);
    if (!marked.is_valid()) {
        registered.error = marked.error;
        return registered;
    }

    registered.font_id = QFontDatabase::addApplicationFontFromData(marked.bytes);
    if (registered.font_id < 0) {
        registered.error = QStringLiteral("The font database refused the marked font %1.")
                               .arg(marked.family);
        return registered;
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(registered.font_id);
    if (families.size() != 1) {
        QFontDatabase::removeApplicationFont(registered.font_id);
        registered.font_id = -1;
        registered.error   = QStringLiteral("The marked font exposed %1 families, not one: %2.")
                                 .arg(families.size()).arg(families.join(QStringLiteral(", ")));
        return registered;
    }

    registered.family = families.first();
    if (!registered.family.endsWith(QString::fromLatin1(k_family_mark))) {
        QFontDatabase::removeApplicationFont(registered.font_id);
        registered.font_id = -1;
        registered.error   = QStringLiteral(
                                 "The font database resolved %1, which does not carry the mark. "
                                 "Registering it would reintroduce the family collision.")
                                 .arg(families.first());
        registered.family.clear();
    }
    return registered;
}

}
