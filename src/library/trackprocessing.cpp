#include "library/trackprocessing.h"

#include <QProgressDialog>
#include <QThread>

#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("ModalTrackBatchProcessor");

} // anonymous namespace

int ModalTrackBatchProcessor::processTracks(
        const QString& progressLabelText,
        TrackCollectionManager* pTrackCollectionManager,
        TrackPointerIterator* pTrackPointerIterator) {
    DEBUG_ASSERT(pTrackCollectionManager);
    DEBUG_ASSERT(pTrackPointerIterator);
    DEBUG_ASSERT(QThread::currentThread() ==
            pTrackCollectionManager->thread());
    int trackCount = 0;
    int estimatedTotalCount =
            pTrackPointerIterator->estimateItemsRemaining().value_or(trackCount);
    QProgressDialog progressDlg(
            progressLabelText,
            tr("Abort"),
            trackCount,
            estimatedTotalCount);
    progressDlg.setWindowModality(
            Qt::ApplicationModal);
    progressDlg.setMinimumDuration(
            m_minimumProgressDuration.toIntegerMillis());
    while (auto nextTrackPointer = pTrackPointerIterator->nextItem()) {
        const auto pTrack = *nextTrackPointer;
        VERIFY_OR_DEBUG_ASSERT(pTrack) {
            kLogger.warning()
                    << progressLabelText
                    << "failed to load next track for processing";
            continue;
        }
        if (progressDlg.wasCanceled()) {
            kLogger.info()
                    << "Aborting"
                    << progressLabelText
                    << "after processing"
                    << trackCount
                    << "of"
                    << estimatedTotalCount
                    << "track(s)";
            return trackCount;
        }
        switch (doProcessNextTrack(pTrack)) {
        case ProcessNextTrackResult::AbortProcessing:
            kLogger.info()
                    << progressLabelText
                    << "aborted while processing"
                    << trackCount + 1
                    << "of"
                    << estimatedTotalCount
                    << "track(s)";
            return trackCount;
        case ProcessNextTrackResult::ContinueProcessing:
            break;
        case ProcessNextTrackResult::SaveTrackAndContinueProcessing:
            pTrackCollectionManager->saveTrack(pTrack);
            break;
        }
        ++trackCount;
        if (trackCount > estimatedTotalCount) {
            estimatedTotalCount =
                    pTrackPointerIterator->estimateItemsRemaining().value_or(trackCount);
            DEBUG_ASSERT(trackCount <= estimatedTotalCount);
            progressDlg.setMaximum(estimatedTotalCount);
        }
        progressDlg.setValue(trackCount);
    }
    return trackCount;
}

ModalTrackBatchOperationProcessor::ModalTrackBatchOperationProcessor(
        const TrackPointerOperation* pTrackPointerOperation,
        Mode mode,
        Duration progressGracePeriod,
        QObject* parent)
        : ModalTrackBatchProcessor(progressGracePeriod, parent),
          m_pTrackPointerOperation(pTrackPointerOperation),
          m_mode(mode) {
    DEBUG_ASSERT(m_pTrackPointerOperation);
}

ModalTrackBatchProcessor::ProcessNextTrackResult
ModalTrackBatchOperationProcessor::doProcessNextTrack(
        const TrackPointer& pTrack) {
    m_pTrackPointerOperation->apply(pTrack);
    switch (m_mode) {
    case Mode::Apply:
        return ProcessNextTrackResult::ContinueProcessing;
    case Mode::ApplyAndSave:
        return ProcessNextTrackResult::SaveTrackAndContinueProcessing;
    }
    DEBUG_ASSERT(!"unreachable");
    return ProcessNextTrackResult::AbortProcessing;
}

} // namespace mixxx
