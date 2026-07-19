// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionInfo_TrackEditor.h"
#include "ISequencer.h"
#include "../Track/Track_ActionInfo.h"
#include "../Section/Section_ActionInfo.h"
#include "SequencerSectionPainter.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FActionInfo_TrackEditor"

FActionInfo_TrackEditor::FActionInfo_TrackEditor(TSharedRef<ISequencer> InSequencer)
    : FMovieSceneTrackEditor(InSequencer)
{
}

FActionInfo_TrackEditor::~FActionInfo_TrackEditor()
{
}

TSharedRef<ISequencerTrackEditor> FActionInfo_TrackEditor::CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
{
    return MakeShareable(new FActionInfo_TrackEditor(OwningSequencer));
}


FText FActionInfo_TrackEditor::GetDisplayName() const
{
    return LOCTEXT("ActionInfo_TrackEditor", "ActionInfo");
}

bool FActionInfo_TrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
    return TrackClass == UTrack_ActionInfo::StaticClass();
}


bool FActionInfo_TrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
    return true;
}


const FSlateBrush* FActionInfo_TrackEditor::GetIconBrush() const
{
    return FAppStyle::GetBrush("DebugConsole.Icon");
}


void FActionInfo_TrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
    // 使用 MenuBuilder 添加一个菜单项
    MenuBuilder.AddMenuEntry(
    LOCTEXT("AddActionInfoTrack", "ActionInfo Track"),
    LOCTEXT("AddActionInfoTrackTooltip", "Adds a new track that defined the action info."),
    FSlateIcon(), // 可以指定一个图标
    FUIAction(
    FExecuteAction::CreateRaw(this, &FActionInfo_TrackEditor::HandleAddTrackMenuEntryExecute),
    FCanExecuteAction::CreateSP(this, &FActionInfo_TrackEditor::CanAddTrack)));
}

void FActionInfo_TrackEditor::HandleAddTrackMenuEntryExecute()
{
    UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

    if (FocusedMovieScene == nullptr)
    {
        return;
    }

    if (FocusedMovieScene->IsReadOnly())
    {
        return;
    }

    const FScopedTransaction Transaction(LOCTEXT("AddActionInfoTrack_Transaction", "Add ActionInfo Track"));
    FocusedMovieScene->Modify();

    UTrack_ActionInfo* NewTrack = FocusedMovieScene->AddTrack<UTrack_ActionInfo>();
    check(NewTrack);

    NewTrack->SetDisplayName(LOCTEXT("ActionInfoTrackName", "ActionInfo"));

    if (GetSequencer().IsValid())
    {
        GetSequencer()->OnAddTrack(NewTrack, FGuid());
    }
}

bool FActionInfo_TrackEditor::CanAddTrack() const
{
    TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
    if (!SequencerPtr.IsValid()) return false;

    UMovieScene* MovieScene = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene();
    if (!MovieScene) return false;

    // 如果已存在则返回 false，按钮变灰
    for (UMovieSceneTrack* Track : MovieScene->GetTracks())
    {
        if (Track && Track->IsA<UTrack_ActionInfo>())
        {
            return false;
        }
    }
    return true;
}

TSharedRef<ISequencerSection> FActionInfo_TrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
    check(SupportsType(SectionObject.GetOuter()->GetClass()));
    return MakeShared<FActionInfo_SectionEditor>(SectionObject, GetSequencer());
}

//#region  section
FActionInfo_SectionEditor::FActionInfo_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
    : Section(&InSection)
    , Sequencer(InSequencer)
{
}

FActionInfo_SectionEditor::~FActionInfo_SectionEditor()
{
}

UMovieSceneSection* FActionInfo_SectionEditor::GetSectionObject()
{
    return Section.Get();
}

FText FActionInfo_SectionEditor::GetSectionTitle() const
{
    return FText::FromString("ActionInfo");
}

FText FActionInfo_SectionEditor::GetSectionToolTip() const
{
    return FText::FromString("Define action info");
}

float FActionInfo_SectionEditor::GetSectionHeight() const
{
    return 40;
}

int32 FActionInfo_SectionEditor::OnPaintSection(FSequencerSectionPainter& Painter) const
{
    return Painter.PaintSectionBackground();
}

void FActionInfo_SectionEditor::BeginResizeSection()
{
}

void FActionInfo_SectionEditor::ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime)
{
    ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}

void FActionInfo_SectionEditor::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder)
{
}

void FActionInfo_SectionEditor::BeginSlipSection()
{
}

void FActionInfo_SectionEditor::SlipSection(FFrameNumber SlipTime)
{
    ISequencerSection::SlipSection(SlipTime);
}
//#endregion

#undef LOCTEXT_NAMESPACE