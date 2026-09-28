#ifndef TEST_PORTABLE_MODE_H
#define TEST_PORTABLE_MODE_H

#include <QObject>

/// §零.3 acceptance: portable.flag semantics + dual-instance isolation.
class TestPortableMode : public QObject
{
    Q_OBJECT
public:
    explicit TestPortableMode(QObject* parent = nullptr) : QObject(parent)
    { }

private slots:
    void testNoFlagMeansStandardDir();
    void testFlagRedirectsToAppDir();
    void testFlagIsCaseInsensitiveNameExact();
    void testTwoPortablesDontPolluteEachOther();
};

#endif // TEST_PORTABLE_MODE_H
