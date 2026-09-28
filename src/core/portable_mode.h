#ifndef PORTABLE_MODE_H
#define PORTABLE_MODE_H

#include <QString>

/// §零.3 portable mode: when a `portable.flag` file sits next to the exe,
/// ALL app data (config.json, converter.log, theme state) relocates from
/// %APPDATA% into the run directory — "数据随目录" (data travels with the
/// folder). Pure functions; no globals; unit-testable without a GUI.
namespace PortableMode
{

/// true iff `portable.flag` exists next to the given application dir.
/// Takes the dir explicitly (not QCoreApplication) so tests can point at a
/// temp dir — same argument style as the §3.2 "assert semantics, not
/// environment" rule.
bool isPortable(const QString& appDirPath);

/// Data dir for the session: portable ? appDirPath : the standard
/// QStandardPaths AppConfigLocation passed in.
QString dataDir(const QString& appDirPath, const QString& standardDir);

/// Convenience over the running application: resolves
/// QCoreApplication::applicationDirPath() + portable check.
QString currentDataDir();

} // namespace PortableMode

#endif // PORTABLE_MODE_H
