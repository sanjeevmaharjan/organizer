#include "core/Organizer.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <filesystem>
#include <stdexcept>

class OrganizerTests final : public QObject {
    Q_OBJECT

private slots:
    void plansFilesByTypeAndIgnoresFoldersAndSymlinks();
    void plansCollisionFreeDestinations();
    void rejectsSymlinkCategoryDirectories();
    void executesMovesWithoutOverwritingExistingFiles();
};

namespace {

void createFile(const std::filesystem::path& path, const QByteArray& contents)
{
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) {
        qFatal("Unable to create test file");
    }
}

}

void OrganizerTests::plansFilesByTypeAndIgnoresFoldersAndSymlinks()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const std::filesystem::path root(temporaryDirectory.path().toStdString());
    createFile(root / "report.PDF", "document");
    createFile(root / "picture.png", "image");
    createFile(root / "unknown", "other");
    std::filesystem::create_directory(root / "existing-folder");
#if defined(Q_OS_UNIX)
    std::filesystem::create_symlink(root / "report.PDF", root / "report-link.PDF");
#endif

    const auto plan = organizer::Organizer::buildPlan(root);
    QCOMPARE(plan.size(), std::size_t(3));
    QCOMPARE(plan[0].category, std::string("Images"));
    QCOMPARE(plan[1].category, std::string("Documents"));
    QCOMPARE(plan[2].category, std::string("Other"));
#if defined(Q_OS_UNIX)
    QVERIFY(std::filesystem::is_symlink(root / "report-link.PDF"));
#endif
}

void OrganizerTests::plansCollisionFreeDestinations()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const std::filesystem::path root(temporaryDirectory.path().toStdString());
    const auto documents = root / "Documents";
    std::filesystem::create_directory(documents);
    createFile(root / "report.txt", "new");
    createFile(documents / "report.txt", "existing");

    const auto plan = organizer::Organizer::buildPlan(root);
    QCOMPARE(plan.size(), std::size_t(1));
    QCOMPARE(plan[0].destination.filename().string(), std::string("report (1).txt"));
}

void OrganizerTests::rejectsSymlinkCategoryDirectories()
{
#if defined(Q_OS_UNIX)
    QTemporaryDir temporaryDirectory;
    QTemporaryDir externalDirectory;
    QVERIFY(temporaryDirectory.isValid());
    QVERIFY(externalDirectory.isValid());
    const std::filesystem::path root(temporaryDirectory.path().toStdString());
    createFile(root / "report.txt", "document");
    std::filesystem::create_directory_symlink(
        std::filesystem::path(externalDirectory.path().toStdString()), root / "Documents");

    QVERIFY_EXCEPTION_THROWN(organizer::Organizer::buildPlan(root), std::invalid_argument);
#endif
}

void OrganizerTests::executesMovesWithoutOverwritingExistingFiles()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const std::filesystem::path root(temporaryDirectory.path().toStdString());
    const auto documents = root / "Documents";
    std::filesystem::create_directory(documents);
    createFile(root / "report.txt", "new");
    createFile(documents / "report.txt", "existing");

    const auto plan = organizer::Organizer::buildPlan(root);
    createFile(documents / "report (1).txt", "created after preview");
    const auto result = organizer::Organizer::execute(plan);

    QCOMPARE(result.completed.size(), std::size_t(1));
    QVERIFY(result.failed.empty());
    QCOMPARE(result.completed.front().destination.filename().string(),
             std::string("report (2).txt"));
    QVERIFY(!std::filesystem::exists(root / "report.txt"));
    QVERIFY(std::filesystem::exists(documents / "report.txt"));
    QVERIFY(std::filesystem::exists(documents / "report (1).txt"));
    QVERIFY(std::filesystem::exists(documents / "report (2).txt"));
}

QTEST_APPLESS_MAIN(OrganizerTests)

#include "OrganizerTests.moc"
