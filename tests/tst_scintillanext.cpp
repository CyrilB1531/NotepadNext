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


#include <QSignalSpy>
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

class TestScintillaNext : public QObject
{
    Q_OBJECT

private slots:
    void setBomMarksDocumentModified();
    void setBomBackToSavedEncodingIsUnmodified();
    void setBomEmitsBomChangedOnlyOnChange();
    void saveAsWritesEncoding_data();
    void saveAsWritesEncoding();
    void reopeningKeepsEncoding_data();
    void reopeningKeepsEncoding();
    void undoToSavePointStaysModifiedWhenEncodingChanged();
};

void TestScintillaNext::setBomMarksDocumentModified()
{
    ScintillaNext editor(QStringLiteral("New 1"));
    QVERIFY(editor.isSavedToDisk());

    editor.setBom(BomType::Utf16LE);

    QCOMPARE(editor.bom(), BomType::Utf16LE);
    QVERIFY(!editor.isSavedToDisk());
    QVERIFY(!editor.modify()); // The text itself did not change
}

void TestScintillaNext::setBomBackToSavedEncodingIsUnmodified()
{
    ScintillaNext editor(QStringLiteral("New 1"));

    editor.setBom(BomType::Utf8);
    editor.setBom(BomType::None);

    QVERIFY(editor.isSavedToDisk());
}

void TestScintillaNext::setBomEmitsBomChangedOnlyOnChange()
{
    ScintillaNext editor(QStringLiteral("New 1"));
    QSignalSpy spy(&editor, &ScintillaNext::bomChanged);

    editor.setBom(BomType::Utf16BE);
    editor.setBom(BomType::Utf16BE);

    QCOMPARE(spy.count(), 1);
}

void TestScintillaNext::saveAsWritesEncoding_data()
{
    QTest::addColumn<BomType>("bom");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("UTF-8") << BomType::None << QByteArray::fromHex("68c3a90a");
    QTest::newRow("UTF-8 BOM") << BomType::Utf8 << QByteArray::fromHex("efbbbf68c3a90a");
    QTest::newRow("UTF-16 LE BOM") << BomType::Utf16LE << QByteArray::fromHex("fffe6800e9000a00");
    QTest::newRow("UTF-16 BE BOM") << BomType::Utf16BE << QByteArray::fromHex("feff006800e9000a");
}

void TestScintillaNext::saveAsWritesEncoding()
{
    QFETCH(BomType, bom);
    QFETCH(QByteArray, expected);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.filePath(QStringLiteral("file.txt"));

    ScintillaNext editor(QStringLiteral("New 1"));
    editor.setText("h\xc3\xa9\n");
    editor.setBom(bom);

    QCOMPARE(editor.saveAs(filePath), QFileDevice::NoError);

    QCOMPARE(readFile(filePath), expected);
    QVERIFY(editor.isSavedToDisk());
}

void TestScintillaNext::reopeningKeepsEncoding_data()
{
    saveAsWritesEncoding_data();
}

void TestScintillaNext::reopeningKeepsEncoding()
{
    QFETCH(BomType, bom);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.filePath(QStringLiteral("file.txt"));

    ScintillaNext editor(QStringLiteral("New 1"));
    editor.setText("h\xc3\xa9\n");
    editor.setBom(bom);
    QCOMPARE(editor.saveAs(filePath), QFileDevice::NoError);

    QScopedPointer<ScintillaNext> reopened(ScintillaNext::fromFile(filePath));

    QCOMPARE(reopened->bom(), bom);
    QCOMPARE(reopened->getText(reopened->textLength()), QByteArray("h\xc3\xa9\n"));
    QVERIFY(reopened->isSavedToDisk());
}

void TestScintillaNext::undoToSavePointStaysModifiedWhenEncodingChanged()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.filePath(QStringLiteral("file.txt"));

    ScintillaNext editor(QStringLiteral("New 1"));
    editor.setText("text\n");
    QCOMPARE(editor.saveAs(filePath), QFileDevice::NoError);

    editor.setBom(BomType::Utf8);
    editor.appendText(1, "x");
    editor.undo();

    QVERIFY(!editor.modify());
    QVERIFY(!editor.isSavedToDisk());
}

QTEST_MAIN(TestScintillaNext)

#include "tst_scintillanext.moc"
