// Fill out your copyright notice in the Description page of Project Settings.

#include "MoveInputAcceptance_TrackEditor.h"
#include "ISequencer.h"
#include "../Track/MoveInputAcceptance/Track_MoveInputAcceptance.h"
#include "../Section/MoveInputAcceptance/Section_MoveInputAcceptance.h"
#include "SequencerSectionPainter.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "FMoveInputAcceptance_TrackEditor"

FMoveInputAcceptance_TrackEditor::FMoveInputAcceptance_TrackEditor(TSharedRef<ISequencer> InSequencer)
    : FMovieSceneTrackEditor(InSequencer)
{
}

FMoveInputAcceptance_TrackEditor::~FMoveInputAcceptance_TrackEditor()
{
}

TSharedRef<ISequencerTrackEditor> FMoveInputAcceptance_TrackEditor::CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
{
    return MakeShareable(new FMoveInputAcceptance_TrackEditor(OwningSequencer));
}


FText FMoveInputAcceptance_TrackEditor::GetDisplayName() const
{
    return LOCTEXT("MoveInputAcceptance_TrackEditor", "MoveInputAcceptance");
}

bool FMoveInputAcceptance_TrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
    return TrackClass == UTrack_MoveInputAcceptance::StaticClass();
}


bool FMoveInputAcceptance_TrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
    return true;
}


const FSlateBrush* FMoveInputAcceptance_TrackEditor::GetIconBrush() const
{
    return FAppStyle::GetBrush("DebugConsole.Icon");
}


void FMoveInputAcceptance_TrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
    // 使用 MenuBuilder 添加一个菜单项
    MenuBuilder.AddMenuEntry(
    LOCTEXT("AddMoveInputAcceptanceTrack", "MoveInputAcceptance Track"),
    LOCTEXT("AddMoveInputAcceptanceTooltip", "Adds a new track that defined the action move input acceptance."),
    FSlateIcon(), // 可以指定一个图标
    FUIAction(
    FExecuteAction::CreateRaw(this, &FMoveInputAcceptance_TrackEditor::HandleAddTrackMenuEntryExecute),
    FCanExecuteAction::CreateSP(this, &FMoveInputAcceptance_TrackEditor::CanAddTrack)));
}

void FMoveInputAcceptance_TrackEditor::HandleAddTrackMenuEntryExecute()
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

    const FScopedTransaction Transaction(LOCTEXT("AddMoveInputAcceptance_Transaction", "Add MoveInputAcceptance Track"));
    FocusedMovieScene->Modify();

    UTrack_MoveInputAcceptance* NewTrack = FocusedMovieScene->AddTrack<UTrack_MoveInputAcceptance>();
    check(NewTrack);

    NewTrack->SetDisplayName(LOCTEXT("MoveInputAcceptanceTrackName", "MoveInputAcceptance"));

    if (GetSequencer().IsValid())
    {
        GetSequencer()->OnAddTrack(NewTrack, FGuid());
    }
}

bool FMoveInputAcceptance_TrackEditor::CanAddTrack() const
{
    TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
    if (!SequencerPtr.IsValid()) return false;

    UMovieScene* MovieScene = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene();
    if (!MovieScene) return false;

    return true;
}

TSharedRef<ISequencerSection> FMoveInputAcceptance_TrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
    check(SupportsType(SectionObject.GetOuter()->GetClass()));
    return MakeShared<FMoveInputAcceptance_SectionEditor>(SectionObject, GetSequencer());
}

//#region  section
FMoveInputAcceptance_SectionEditor::FMoveInputAcceptance_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
    : Section(&InSection)
    , Sequencer(InSequencer)
{
}

FMoveInputAcceptance_SectionEditor::~FMoveInputAcceptance_SectionEditor()
{
}

UMovieSceneSection* FMoveInputAcceptance_SectionEditor::GetSectionObject()
{
    return Section.Get();
}

FText FMoveInputAcceptance_SectionEditor::GetSectionTitle() const
{
    return FText::FromString("MoveInputAcceptance");
}

FText FMoveInputAcceptance_SectionEditor::GetSectionToolTip() const
{
    return FText::FromString("Define action move input acceptance info");
}

float FMoveInputAcceptance_SectionEditor::GetSectionHeight() const
{
    return 40;
}

int32 FMoveInputAcceptance_SectionEditor::OnPaintSection(FSequencerSectionPainter& Painter) const
{
    return Painter.PaintSectionBackground();
}

void FMoveInputAcceptance_SectionEditor::BeginResizeSection()
{
}

void FMoveInputAcceptance_SectionEditor::ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime)
{
    ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}

void FMoveInputAcceptance_SectionEditor::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder)
{
}

void FMoveInputAcceptance_SectionEditor::BeginSlipSection()
{
}

void FMoveInputAcceptance_SectionEditor::SlipSection(FFrameNumber SlipTime)
{
    ISequencerSection::SlipSection(SlipTime);
}
//#endregion

#undef LOCTEXT_NAMESPACE