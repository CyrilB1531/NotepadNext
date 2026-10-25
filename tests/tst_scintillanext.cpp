/*
 * This file is part of Notepad Next.
 * Copyright 2026 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <QTemporaryDir>
#include <QTest>

#include "ScintillaNext.h"

using BomType = ScintillaNext::BomType;

Q_DECLARE_METATYPE(ScintillaNext::BomType)


static QByteArray readFile(const QString &filePath)
{
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    return file.readAll();
}

static void writeFile(const QString &filePath, const QByteArray &data)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(data);
}

class TestScintillaNext : public QObject
{
    Q_OBJECT

private slots:
    void bomTypeStringRoundTrip_data();
    void bomTypeStringRoundTrip();
    void stringToBomTypeRejectsUnknown_data();
    void stringToBomTypeRejectsUnknown();
    void setBomOnNewDocumentStaysUnmodified();
    void setBomOnFileMarksModified();
    void newDocumentIsSavedInItsEncoding_data();
    void newDocumentIsSavedInItsEncoding();
};

void TestScintillaNext::bomTypeStringRoundTrip_data()
{
    QTest::addColumn<BomType>("bom");
    QTest::addColumn<QString>("string");

    QTest::newRow("UTF-8") << BomType::None << QStringLiteral("utf-8");
    QTest::newRow("UTF-8 BOM") << BomType::Utf8 << QStringLiteral("utf-8-bom");
    QTest::newRow("UTF-16 BE BOM") << BomType::Utf16BE << QStringLiteral("utf-16be-bom");
    QTest::newRow("UTF-16 LE BOM") << BomType::Utf16LE << QStringLiteral("utf-16le-bom");
}

void TestScintillaNext::bomTypeStringRoundTrip()
{
    QFETCH(BomType, bom);
    QFETCH(QString, string);

    QCOMPARE(ScintillaNext::bomTypeToString(bom), string);

    bool ok = false;
    QCOMPARE(ScintillaNext::stringToBomType(string, &ok), bom);
    QVERIFY(ok);
}

void TestScintillaNext::stringToBomTypeRejectsUnknown_data()
{
    QTest::addColumn<QString>("string");

    QTest::newRow("empty") << QString();
    QTest::newRow("unknown") << QStringLiteral("latin-1");
    QTest::newRow("upper case") << QStringLiteral("UTF-8-BOM"); // Callers are expected to lower the case
}

void TestScintillaNext::stringToBomTypeRejectsUnknown()
{
    QFETCH(QString, string);

    bool ok = true;
    QCOMPARE(ScintillaNext::stringToBomType(string, &ok), BomType::None);
    QVERIFY(!ok);
}

void TestScintillaNext::setBomOnNewDocumentStaysUnmodified()
{
    ScintillaNext editor(QStringLiteral("New 1"));

    editor.setBom(BomType::Utf16LE);

    // An empty new document has nothing to save, whatever its encoding
    QCOMPARE(editor.bom(), BomType::Utf16LE);
    QVERIFY(editor.isSavedToDisk());

    editor.setText("text");
    QVERIFY(!editor.isSavedToDisk());
}

void TestScintillaNext::setBomOnFileMarksModified()
{
    QTemporaryDir dir;
    const QString filePath = dir.filePath(QStringLiteral("file.txt"));
    writeFile(filePath, "text");

    QScopedPointer<ScintillaNext> editor(ScintillaNext::fromFile(filePath));
    QVERIFY(editor->isSavedToDisk());

    editor->setBom(BomType::Utf8);

    // The file on disk has to be written again in the new encoding
    QVERIFY(!editor->isSavedToDisk());
}

void TestScintillaNext::newDocumentIsSavedInItsEncoding_data()
{
    QTest::addColumn<BomType>("bom");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("UTF-8") << BomType::None << QByteArray("h\xc3\xa9");
    QTest::newRow("UTF-8 BOM") << BomType::Utf8 << QByteArray("\xef\xbb\xbfh\xc3\xa9");
    QTest::newRow("UTF-16 BE BOM") << BomType::Utf16BE << QByteArray("\xfe\xff\x00h\x00\xe9", 6);
    QTest::newRow("UTF-16 LE BOM") << BomType::Utf16LE << QByteArray("\xff\xfeh\x00\xe9\x00", 6);
}

void TestScintillaNext::newDocumentIsSavedInItsEncoding()
{
    QFETCH(BomType, bom);
    QFETCH(QByteArray, expected);

    QTemporaryDir dir;
    const QString filePath = dir.filePath(QStringLiteral("new.txt"));

    ScintillaNext editor(QStringLiteral("New 1"));
    editor.setBom(bom);
    editor.setText("h\xc3\xa9");

    QCOMPARE(editor.saveAs(filePath), QFileDevice::NoError);
    QCOMPARE(readFile(filePath), expected);
    QVERIFY(editor.isSavedToDisk());
}

QTEST_MAIN(TestScintillaNext)

#include "tst_scintillanext.moc"
