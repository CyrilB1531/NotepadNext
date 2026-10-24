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


#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include "ScintillaNext.h"


class TestScintillaNext : public QObject
{
    Q_OBJECT

private slots:
    void toolTipOfNewBufferIsName();
    void toolTipOfFileIsFullPath();
    void toolTipFollowsSaveAs();
    void toolTipOfDetachedFileIsNewName();
};

void TestScintillaNext::toolTipOfNewBufferIsName()
{
    ScintillaNext editor(QStringLiteral("New 1"));

    QCOMPARE(editor.getToolTip(), QStringLiteral("New 1"));
}

void TestScintillaNext::toolTipOfFileIsFullPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = dir.filePath(QStringLiteral("index.html"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<html></html>");
    file.close();

    QScopedPointer<ScintillaNext> editor(ScintillaNext::fromFile(filePath));

    QCOMPARE(editor->getToolTip(), QDir::toNativeSeparators(QFileInfo(filePath).canonicalFilePath()));
}

void TestScintillaNext::toolTipFollowsSaveAs()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = dir.filePath(QStringLiteral("saved.txt"));

    ScintillaNext editor(QStringLiteral("New 1"));
    editor.setText("content");
    QCOMPARE(editor.saveAs(filePath), QFileDevice::NoError);

    QCOMPARE(editor.getToolTip(), QDir::toNativeSeparators(QFileInfo(filePath).canonicalFilePath()));
}

void TestScintillaNext::toolTipOfDetachedFileIsNewName()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = dir.filePath(QStringLiteral("removed.txt"));

    ScintillaNext editor(QStringLiteral("New 1"));
    QCOMPARE(editor.saveAs(filePath), QFileDevice::NoError);

    editor.detachFileInfo(QStringLiteral("New 2"));

    QCOMPARE(editor.getToolTip(), QStringLiteral("New 2"));
}

QTEST_MAIN(TestScintillaNext)

#include "tst_scintillanext.moc"
