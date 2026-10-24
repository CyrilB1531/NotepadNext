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


#include <QTest>

#include "ScintillaNext.h"


class TestScintillaNext : public QObject
{
    Q_OBJECT

private slots:
    void trimTrailingWhitespace_data();
    void trimTrailingWhitespace();
    void trimTrailingWhitespaceIsSingleUndoStep();
    void trimTrailingWhitespaceKeepsCleanDocumentUnmodified();
};

void TestScintillaNext::trimTrailingWhitespace_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("LF") << QByteArray("a  \nb\t \n  c\n") << QByteArray("a\nb\n  c\n");
    QTest::newRow("CRLF") << QByteArray("a  \r\nb\t \r\n  c\r\n") << QByteArray("a\r\nb\r\n  c\r\n");
    QTest::newRow("CR") << QByteArray("a  \rb\t \r  c\r") << QByteArray("a\rb\r  c\r");
    QTest::newRow("no final EOL") << QByteArray("a\nb   ") << QByteArray("a\nb");
    QTest::newRow("whitespace only lines") << QByteArray("  \n\t\r\n   ") << QByteArray("\n\r\n");
    QTest::newRow("inner spaces kept") << QByteArray("a b  c \n") << QByteArray("a b  c\n");
    QTest::newRow("nothing to trim") << QByteArray("abc\r\ndef\r\n") << QByteArray("abc\r\ndef\r\n");
    QTest::newRow("empty document") << QByteArray("") << QByteArray("");
    QTest::newRow("unicode") << QByteArray("é😀 \t\r\nü  ") << QByteArray("é😀\r\nü");
}

void TestScintillaNext::trimTrailingWhitespace()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, expected);

    ScintillaNext editor(QStringLiteral("test"));
    editor.setText(input.constData());

    editor.trimTrailingWhitespace();

    QCOMPARE(editor.getText(editor.textLength()), expected);
}

void TestScintillaNext::trimTrailingWhitespaceIsSingleUndoStep()
{
    const QByteArray input("a  \nb\t \nc   ");

    ScintillaNext editor(QStringLiteral("test"));
    editor.setText(input.constData());
    editor.emptyUndoBuffer();

    editor.trimTrailingWhitespace();
    editor.undo();

    QCOMPARE(editor.getText(editor.textLength()), input);
    QVERIFY(!editor.canUndo());
}

void TestScintillaNext::trimTrailingWhitespaceKeepsCleanDocumentUnmodified()
{
    ScintillaNext editor(QStringLiteral("test"));
    editor.setText("clean\nfile\n");
    editor.emptyUndoBuffer();
    editor.setSavePoint();

    editor.trimTrailingWhitespace();

    QVERIFY(!editor.modify());
    QVERIFY(!editor.canUndo());
}

QTEST_MAIN(TestScintillaNext)

#include "tst_scintillanext.moc"
