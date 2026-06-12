#ifndef TEST_SKILL_MANAGER_H
#define TEST_SKILL_MANAGER_H

#include <QObject>
#include <QString>

class TestSkillManager : public QObject {
    Q_OBJECT
public:
    explicit TestSkillManager(QObject* parent = nullptr) : QObject(parent) {}
private slots:
    void testSingletonInstance();
    void testAvailableSkills();
    void testCategories();
    void testSkillInfo();
    void testSkillDescription();
    void testSkillDescriptionUnknown();
    void testIsSkillRunning();
    void testRegisterCustomSkill();
    void testUnregisterCustomSkill();
    void testSkillsByCategory();
    void testInvokeUnknownSkill();
};

#endif // TEST_SKILL_MANAGER_H
