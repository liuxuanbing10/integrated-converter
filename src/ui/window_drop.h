#ifndef WINDOW_DROP_H
#define WINDOW_DROP_H

#include <QStringList>

class QMimeData;
class QDropEvent;

namespace WindowDrop
{

/// §零.2 assembly extraction: URL-payload gate shared by dragEnter/dragMove.
/// Takes the mime data (not the event) so no QEvent-subclass coupling is
/// needed across Qt point releases.
bool hasLocalFileUrls(const QMimeData* mimeData);

/// Local file URLs -> paths; a dropped folder expands to its immediate
/// children (what users expect from Explorer).
QStringList pathsFromDrop(QDropEvent* event);

} // namespace WindowDrop

#endif // WINDOW_DROP_H
