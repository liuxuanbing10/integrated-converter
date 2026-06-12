#include "test_skill_manager.h"
#include "core/skill_manager.h"
#include <QTest>
#include <QSignalSpy>

void TestSkillManager::testSingletonInstance() {
    SkillManager* s1 = SkillManager::instance();
    SkillManager* s2 = SkillManager::instance();
    QVERIFY(s1 != nullptr);
    QVERIFY(s1 == s2);
}

void TestSkillManager::testAvailableSkills() {
    SkillManager* mgr = SkillManager::instance();
    QStringList skills = mgr->availableSkills();
    QCOMPARE(skills.size(), 5);
    QVERIFY(skills.contains("format_detect"));
    QVERIFY(skills.contains("batch_rename"));
    QVERIFY(skills.contains("preset_apply"));
    QVERIFY(skills.contains("history_export"));
    QVERIFY(skills.contains("file_validate"));
}

void TestSkillManager::testCategories() {
    SkillManager* mgr = SkillManager::instance();
    QStringList cats = mgr->categories();
    QVERIFY(cats.contains(QString::fromUtf8("\xe5\x88\x86\xe6\x9e\x90")));
    QVERIFY(cats.contains(QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe6\x93\x8d\xe4\xbd\x9c")));
    QVERIFY(cats.contains(QString::fromUtf8("\xe8\xbd\xac\xe6\x8d\xa2")));
    QVERIFY(cats.contains(QString::fromUtf8("\xe5\x8e\x86\xe5\x8f\xb2")));
}

void TestSkillManager::testSkillInfo() {
    SkillManager* mgr = SkillManager::instance();
    SkillInfo info = mgr->skillInfo("format_detect");
    QCOMPARE(info.name, QString("format_detect"));
    QVERIFY(info.isBuiltIn);
    QVERIFY(!info.description.isEmpty());
}

void TestSkillManager::testSkillDescription() {
    SkillManager* mgr = SkillManager::instance();
    QString desc = mgr->skillDescription("format_detect");
    QVERIFY(!desc.isEmpty());
}

void TestSkillManager::testSkillDescriptionUnknown() {
    SkillManager* mgr = SkillManager::instance();
    QVERIFY(mgr->skillDescription("nonexistent_skill").isEmpty());
}

void TestSkillManager::testIsSkillRunning() {
    SkillManager* mgr = SkillManager::instance();
    QVERIFY(!mgr->isSkillRunning("nonexistent_id"));
}

void TestSkillManager::testRegisterCustomSkill() {
    SkillManager* mgr = SkillManager::instance();
    SkillInfo customSkill;
    customSkill.name = "test_custom_skill";
    customSkill.description = "A test skill";
    customSkill.category = "测试";
    customSkill.isBuiltIn = false;
    int beforeCount = mgr->availableSkills().size();
    mgr->registerCustomSkill(customSkill);
    QCOMPARE(mgr->availableSkills().size(), beforeCount + 1);
    QVERIFY(mgr->availableSkills().contains("test_custom_skill"));
    mgr->unregisterCustomSkill("test_custom_skill");
}

void TestSkillManager::testUnregisterCustomSkill() {
    SkillManager* mgr = SkillManager::instance();
    SkillInfo skill;
    skill.name = "temp_skill";
    skill.isBuiltIn = false;
    mgr->registerCustomSkill(skill);
    int beforeCount = mgr->availableSkills().size();
    mgr->unregisterCustomSkill("temp_skill");
    QCOMPARE(mgr->availableSkills().size(), beforeCount - 1);
}

void TestSkillManager::testSkillsByCategory() {
    SkillManager* mgr = SkillManager::instance();
    QStringList analysis = mgr->skillsByCategory(QString::fromUtf8("\xe5\x88\x86\xe6\x9e\x90"));
    QVERIFY(analysis.contains("format_detect"));
    QVERIFY(analysis.contains("file_validate"));
}

void TestSkillManager::testInvokeUnknownSkill() {
    SkillManager* mgr = SkillManager::instance();
    QSignalSpy spy(mgr, &SkillManager::skillError);
    mgr->invokeSkill("nonexistent_skill");
    QCOMPARE(spy.count(), 1);
}
