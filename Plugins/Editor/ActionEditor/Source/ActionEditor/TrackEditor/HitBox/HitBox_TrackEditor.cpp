// Fill out your copyright notice in the Description page of Project Settings.

#include "HitBox_TrackEditor.h"
#include "ISequencer.h"
#include "../Track/HitBox/Track_HitBox.h"
#include "../Section/HitBox/Section_HitBox.h"
#include "SequencerSectionPainter.h"
#include "EditorStyleSet.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/SCompoundWidget.h"
#include "AnimatedRange.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/ViewModels/TrackAreaViewModel.h"
#include "Misc/FrameNumber.h"

#define LOCTEXT_NAMESPACE "FHitBox_TrackEditor"

FHitBox_TrackEditor::FHitBox_TrackEditor(TSharedRef<ISequencer> InSequencer)
    : FMovieSceneTrackEditor(InSequencer)
{
}

FHitBox_TrackEditor::~FHitBox_TrackEditor()
{
}

TSharedRef<ISequencerTrackEditor> FHitBox_TrackEditor::CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
{
    return MakeShareable(new FHitBox_TrackEditor(OwningSequencer));
}


FText FHitBox_TrackEditor::GetDisplayName() const
{
    return LOCTEXT("HitBox_TrackEditor", "Command");
}

bool FHitBox_TrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
    return TrackClass == UTrack_HitBox::StaticClass();
}

bool FHitBox_TrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
    return true;
}

const FSlateBrush* FHitBox_TrackEditor::GetIconBrush() const
{
    return FAppStyle::GetBrush("DebugConsole.Icon");
}

void FHitBox_TrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
    // 使用 MenuBuilder 添加一个菜单项
    MenuBuilder.AddMenuEntry(
    LOCTEXT("AddHitBoxTrack", "HitBox Track"),
    LOCTEXT("AddHitBoxTrackTooltip", "Adds a new track that defined the hit box."),
    FSlateIcon(), // 可以指定一个图标
    FUIAction(
    FExecuteAction::CreateRaw(this, &FHitBox_TrackEditor::HandleAddTrackMenuEntryExecute),
    FCanExecuteAction::CreateSP(this, &FHitBox_TrackEditor::CanAddTrack)));
}

void FHitBox_TrackEditor::HandleAddTrackMenuEntryExecute()
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

    const FScopedTransaction Transaction(LOCTEXT("AddHitBoxTrack_Transaction", "Add HitBox Track"));
    FocusedMovieScene->Modify();

    UTrack_HitBox* NewTrack = FocusedMovieScene->AddTrack<UTrack_HitBox>();
    check(NewTrack);

    NewTrack->SetDisplayName(LOCTEXT("HitBoxTrackName", "HitBox"));

    if (GetSequencer().IsValid())
    {
        GetSequencer()->OnAddTrack(NewTrack, FGuid());
    }
}

bool FHitBox_TrackEditor::CanAddTrack() const
{
    TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
    if (!SequencerPtr.IsValid()) return false;

    UMovieScene* MovieScene = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene();
    if (!MovieScene) return false;

    // 如果已存在则返回 false, 按钮变灰
    for (UMovieSceneTrack* Track : MovieScene->GetTracks())
    {
        if (Track && Track->IsA<UTrack_HitBox>())
        {
            return false;
        }
    }
    return true;
}

TSharedRef<ISequencerSection> FHitBox_TrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
    check(SupportsType(SectionObject.GetOuter()->GetClass()));
    return MakeShared<FHitBox_SectionEditor>(SectionObject, GetSequencer());
}

class SHitBoxSectionWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHitBoxSectionWidget) {}
    SLATE_ARGUMENT(USection_HitBox*, Section)
    SLATE_ARGUMENT(TWeakPtr<ISequencer>, Sequencer)
    SLATE_ARGUMENT(TWeakPtr<FHitBox_SectionEditor>, SectionEditor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        Section       = InArgs._Section;
        WeakSequencer = InArgs._Sequencer;
        SectionEditor = InArgs._SectionEditor;
    }

    // 鼠标按键处理
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && Section)
        {
            FFrameNumber Frame = FindKeyframeAtPosition(MyGeometry, MouseEvent);
            if (Frame.Value >= 0)
            {
                AHitBoxActor* ExistingActor = Section->GetActorForFrame(Frame);

                // 构建菜单
                FMenuBuilder MenuBuilder(true, nullptr);

                MenuBuilder.AddMenuEntry(
                LOCTEXT("AddActor", "在此关键帧添加预览 Actor"),
                LOCTEXT("AddActorTooltip", "在场景中生成 Actor"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(this, &SHitBoxSectionWidget::AddActorForFrame, Frame)));

                MenuBuilder.AddMenuEntry(
                LOCTEXT("RemoveActor", "删除此关键帧的 Actor"),
                LOCTEXT("RemoveActorTooltip", "从场景中移除 Actor"),
                FSlateIcon(),
                FUIAction(
                FExecuteAction::CreateSP(this, &SHitBoxSectionWidget::RemoveActorForFrame, Frame),
                FCanExecuteAction::CreateLambda([ExistingActor]
                                                { return ExistingActor != nullptr; })));

                FSlateApplication::Get().PushMenu(
                AsShared(),
                FWidgetPath(),
                MenuBuilder.MakeWidget(),
                MouseEvent.GetScreenSpacePosition(),
                FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

                return FReply::Handled();
            }
        }
        return FReply::Unhandled();
    }

private:
    // 辅助：根据鼠标位置查找命中的关键帧
    FFrameNumber FindKeyframeAtPosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
    {
        if (!Section || Section->Keyframes.Num() == 0)
            return FFrameNumber(-1);

        TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
        if (!Sequencer.IsValid())
            return FFrameNumber(-1);

        TSharedPtr<FHitBox_SectionEditor> SectionEditorPtr = SectionEditor.Pin();
        if (!SectionEditorPtr.IsValid())
        {
            return FFrameNumber(-1);
        }


        auto TimeToPixelConverter  = SectionEditorPtr->GetTimeConverter();
        const FVector2D LocalMouse = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const float MousePixelX    = LocalMouse.X;

        const TRange<FFrameNumber> Range = Section->GetRange();
        if (!Range.HasLowerBound())
            return FFrameNumber(-1);

        const float StartPixel   = TimeToPixelConverter.FrameToPixel(Range.GetLowerBoundValue());
        const float DiamondSize  = 6.0f;
        const float HitThreshold = DiamondSize * 2.0f;

        for (const FCapsuleKeyframeData& Key : Section->Keyframes)
        {
            float KeyPixel = TimeToPixelConverter.FrameToPixel(FFrameTime(Key.Time)) - StartPixel;
            if (FMath::Abs(KeyPixel - MousePixelX) < HitThreshold)
            {
                return Key.Time;
            }
        }
        return FFrameNumber(-1);
    }

    // 添加/删除 Actor
    void AddActorForFrame(FFrameNumber Frame)
    {
        if (!Section) return;
        if (Section->GetActorForFrame(Frame)) return;

        //int32 Index = Section->GetKeyframeIndexAtTime(Frame, 0.0f);
        //if (Index == INDEX_NONE) return;
        //const FCapsuleKeyframeData& KeyData = Section->Keyframes[Index];

        //UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        //if (!World) return;

        //// 这里使用 AStaticMeshActor 作为示例，您可替换为自定义 Actor
        //AStaticMeshActor* NewActor = World->SpawnActor<AStaticMeshActor>(KeyData.Location, KeyData.Rotation);
        //if (NewActor)
        //{
        //    NewActor->Tags.Add("HitBoxPreview");
        //    Section->SetActorForFrame(Frame, NewActor);
        //    Section->MarkPackageDirty();

        //    if (auto Sequencer = WeakSequencer.Pin())
        //        Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
        //}
    }

    void RemoveActorForFrame(FFrameNumber Frame)
    {

    }

    USection_HitBox* Section;
    TWeakPtr<ISequencer> WeakSequencer;
    TWeakPtr<FHitBox_SectionEditor> SectionEditor;
};

//#region  section
FHitBox_SectionEditor::FHitBox_SectionEditor(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
    : FSequencerSection(InSection)
    , WeakSequencer(InSequencer)
{
}

FHitBox_SectionEditor::~FHitBox_SectionEditor()
{
}

int32 FHitBox_SectionEditor::OnPaintSection(FSequencerSectionPainter& Painter) const
{
    TimeToPixelConvertor.Valid = false;
    // 1. 先调用父类绘制默认的背景条
    int32 LayerId = FSequencerSection::OnPaintSection(Painter);

    // 2. 获取 Section 数据
    USection_HitBox* CapsuleSection = Cast<USection_HitBox>(WeakSection.Get());
    if (!CapsuleSection || CapsuleSection->Keyframes.Num() == 0)
    {
        return LayerId;
    }

    // 3. 获取 Sequencer 和必要的绘制信息
    TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
    if (!Sequencer.IsValid())
    {
        return LayerId;
    }
    if (!CapsuleSection->HasStartFrame() || !CapsuleSection->HasEndFrame())
    {
        return LayerId;
    }

    // 从 InPainter 中获取绘制所需的几何信息和裁剪矩形
    const FGeometry& AllottedGeometry        = Painter.SectionGeometry;
    //const FSlateRect& MyClippingRect         = Painter.SectionClippingRect;
    FSlateWindowElementList& OutDrawElements = Painter.DrawElements;
    const FFrameRate TickResolution          = Sequencer->GetFocusedTickResolution();
    const FTimeToPixel& TimeToPixel          = Painter.GetTimeConverter();

    auto Temp = const_cast<FTimeToPixel*>(&TimeToPixel);
    TimeToPixelConvertor = *static_cast<UE::Sequencer::FTimeToPixelSpace*>(Temp);
    TimeToPixelConvertor.Valid = true;
    //TimeToPixelConvertor = FTimeToPixelSpaceWarp(AllottedGeometry, Sequencer->GetViewModel()->GetTrackArea()->GetViewRange(), TickResolution);


    // 4. 获取 Section 在时间轴上的范围
    const TRange<FFrameNumber> SectionRange = CapsuleSection->GetRange();
    const FFrameNumber StartFrame           = SectionRange.GetLowerBound().IsClosed() ? SectionRange.GetLowerBoundValue() : TNumericLimits<int32>::Min();
    const FFrameNumber EndFrame             = SectionRange.GetUpperBound().IsClosed() ? SectionRange.GetUpperBoundValue() : TNumericLimits<int32>::Max();

    // 5. 遍历所有关键帧并绘制菱形
    const float SectionHeight        = AllottedGeometry.GetLocalSize().Y;
    const float DiamondSize          = 6.0f;
    const FSlateBrush* KeyframeBrush = FAppStyle::GetBrush("Sequencer.Keyframe");
    const float StartPixelX          = TimeToPixel.FrameToPixel(StartFrame);
    const int32 KeyLayerId           = LayerId + 1;

    for (const FCapsuleKeyframeData& Key : CapsuleSection->Keyframes)
    {
        const FFrameTime KeyTime(Key.Time);
        // 裁剪：如果关键帧不在 Section 范围内，跳过
        if (Key.Time < StartFrame || Key.Time > EndFrame)
        {
            continue;
        }

        // 将帧时间转换为像素坐标
        const float OffsetX = TimeToPixel.FrameToPixel(KeyTime) - StartPixelX;
        // 如果关键帧绘制在可见区域之外，跳过
        if (OffsetX < 0.0f || OffsetX > AllottedGeometry.GetLocalSize().X)
        {
            continue;
        }

        // 绘制菱形
        const FVector2D DiamondCenter(OffsetX, SectionHeight * 0.5f);
        const FVector2D DiamondSizeVec(DiamondSize, DiamondSize);
        const FVector2D DiamondPos = DiamondCenter - DiamondSizeVec * 0.5f;

        FLinearColor KeyColor = FLinearColor::White;
        if (CapsuleSection->GetActorForFrame(Key.Time))
        {
            KeyColor = FLinearColor::Green; // 或其它醒目颜色
        }
        FSlateDrawElement::MakeBox(
        OutDrawElements,
        KeyLayerId, // 高于背景
        AllottedGeometry.ToPaintGeometry(DiamondPos, DiamondSizeVec),
        //Painter.SectionGeometry.ToPaintGeometry(DiamondCenter - DiamondSizeVec * 0.5f, DiamondSizeVec),
        KeyframeBrush,
        ESlateDrawEffect::None,
        KeyColor);
    }

    return KeyLayerId;
}

void FHitBox_SectionEditor::BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding)
{
    MenuBuilder.AddMenuEntry(
    LOCTEXT("AddKeyframe", "在此位置添加关键帧"),
    LOCTEXT("AddKeyframeTooltip", "在当前鼠标位置创建一个新的胶囊体关键帧"),
    FSlateIcon(),
    FUIAction(
    FExecuteAction::CreateRaw(this, &FHitBox_SectionEditor::ShowAddKeyframeDialog)));
}

TSharedRef<SWidget> FHitBox_SectionEditor::GenerateSectionWidget()
{
    USection_HitBox* Section         = Cast<USection_HitBox>(WeakSection.Get());
    TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
    if (Section && Sequencer.IsValid())
    {
        return SNew(SHitBoxSectionWidget)
        .Section(Section)
        .Sequencer(WeakSequencer)
        .SectionEditor(AsShared());
    }
    return SNullWidget::NullWidget;
}

FText FHitBox_SectionEditor::GetSectionTitle() const
{
    return FText::FromString("HitBox");
}

FText FHitBox_SectionEditor::GetSectionToolTip() const
{
    return FText::FromString("Define action hit box info");
}

float FHitBox_SectionEditor::GetSectionHeight() const
{
    return 40;
}

void FHitBox_SectionEditor::BeginResizeSection()
{
}

void FHitBox_SectionEditor::ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime)
{
    ISequencerSection::ResizeSection(ResizeMode, ResizeTime);
}

void FHitBox_SectionEditor::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder)
{
}

void FHitBox_SectionEditor::BeginSlipSection()
{
}

void FHitBox_SectionEditor::SlipSection(FFrameNumber SlipTime)
{
    ISequencerSection::SlipSection(SlipTime);
}

void FHitBox_SectionEditor::ShowAddKeyframeDialog()
{
    TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
    if (!Sequencer.IsValid()) return;

    USection_HitBox* Section = Cast<USection_HitBox>(WeakSection.Get());

    // 获取当前播放头位置作为默认帧号
    const FFrameNumber CurrentFrame = Sequencer->GetLocalTime().Time.FrameNumber;
    int32 DefaultFrame              = CurrentFrame.Value;

    // 创建模态窗口
    TSharedRef<SWindow> ModalWindow = SNew(SWindow)
                                      .Title(LOCTEXT("AddKeyframeDialogTitle", "添加胶囊体关键帧"))
                                      .ClientSize(FVector2D(300, 100))
                                      .SizingRule(ESizingRule::FixedSize)
                                      .SupportsMinimize(false)
                                      .SupportsMaximize(false)
                                      .HasCloseButton(true);

    // 用于存储用户输入的帧号
    const TRange<FFrameNumber> SectionRange        = Section->GetRange();
    const FFrameNumber StartFrame                  = SectionRange.GetLowerBound().IsClosed() ? SectionRange.GetLowerBoundValue() : TNumericLimits<int32>::Min();
    const FFrameNumber EndFrame                    = SectionRange.GetUpperBound().IsClosed() ? SectionRange.GetUpperBoundValue() : TNumericLimits<int32>::Max();
    if (DefaultFrame < StartFrame.Value)
    {
        DefaultFrame = StartFrame.Value;
    }
    auto TickResolution              = Sequencer->GetFocusedTickResolution();
    auto DisplayRate                 = Sequencer->GetFocusedDisplayRate();
    FFrameTime DisplayStartFrameTime = ConvertFrameTime(StartFrame, TickResolution, DisplayRate);
    FFrameTime DisplayEndFrameTime   = ConvertFrameTime(EndFrame, TickResolution, DisplayRate);
    int32 InputFrame                 = ConvertFrameTime(DefaultFrame, TickResolution, DisplayRate).FloorToFrame().Value;
 
    TSharedRef<SNumericEntryBox<int32>> NumericBox = SNew(SNumericEntryBox<int32>)
                                                     .AllowSpin(true)
                                                     .MinValue(DisplayStartFrameTime.FrameNumber.Value) // 无最小值限制，可以自行添加
                                                     .MaxValue(DisplayEndFrameTime.FrameNumber.Value - 1)
                                                     .Value_Lambda([&InputFrame]() -> TOptional<int32>
                                                                   { return InputFrame; })
                                                     .OnValueChanged_Lambda([&InputFrame](int32 NewValue)
                                                                            { InputFrame = NewValue; })
                                                     .Label()
                                                     [SNew(STextBlock)
                                                      .Text(NSLOCTEXT("Sequencer", "FrameNumberLabel", "帧号:"))];

    auto OnClickConform = [this, &InputFrame, ModalWindow]() -> FReply
    {
        // 添加关键帧
        OnAddKeyFrame(FFrameNumber(InputFrame));
        ModalWindow->RequestDestroyWindow();
        UE_LOG(LogTemp, Log, TEXT("确定"));
        return FReply::Handled();
    };
    // 确认按钮
    TSharedRef<SButton> ConfirmButton = SNew(SButton)
                                        .Text(NSLOCTEXT("Sequencer", "Confirm", "确认"))
                                        .OnClicked(FOnClicked::CreateLambda(OnClickConform));


    // 取消按钮
    TSharedRef<SButton> CancelButton = SNew(SButton)
                                       .Text(NSLOCTEXT("Sequencer", "Cancel", "取消"))
                                       .OnClicked_Lambda([ModalWindow]() -> FReply
                                                         {
                                                            ModalWindow->RequestDestroyWindow();
                                                            return FReply::Handled();
                                                         });

    // 布局
    ModalWindow->SetContent(
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)[NumericBox]
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5)[ConfirmButton]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5)[CancelButton]
        ]
    );

    // 显示模态窗口
    FSlateApplication::Get().AddModalWindow(ModalWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void FHitBox_SectionEditor::OnAddKeyFrame(FFrameNumber InDisplayFram)
{
    UE_LOG(LogTemp, Log, TEXT("确定"));

    USection_HitBox* CapsuleSection = Cast<USection_HitBox>(WeakSection.Get());
    if (!CapsuleSection) return;

    TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
    if (!Sequencer.IsValid()) return;

    auto TickResolution  = Sequencer->GetFocusedTickResolution();
    auto DisplayRate     = Sequencer->GetFocusedDisplayRate();
    FFrameTime FrameTime = ConvertFrameTime(InDisplayFram, DisplayRate, TickResolution);

    // 检查是否已存在相同帧的关键帧
    int32 ExistingIndex = CapsuleSection->GetKeyframeIndexAtTime(FrameTime.FrameNumber, 0.0f);
    if (ExistingIndex != INDEX_NONE)
    {
        // 如果存在，可以提示或更新，这里选择更新（覆盖）
        FCapsuleKeyframeData& ExistingKey = CapsuleSection->Keyframes[ExistingIndex];
        // 保持原有参数，但也可以重置为默认值，根据需求决定
        // 这里不修改参数，仅提示已存在
        FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
                                              LOCTEXT("KeyframeExists", "帧 {0} 已存在关键帧，请使用其他帧号。"),
                                              FText::AsNumber(InDisplayFram.Value)));
        return;
    }

    // 创建新关键帧，使用默认参数
    FCapsuleKeyframeData NewKey;
    NewKey.Time              = FrameTime.FrameNumber;
    NewKey.Location          = FVector::ZeroVector;
    NewKey.CapsuleRadius     = 34.0f;
    NewKey.CapsuleHalfHeight = 88.0f;
    NewKey.Rotation          = FRotator::ZeroRotator;

    CapsuleSection->Keyframes.Add(NewKey);
    // 按时间排序
    CapsuleSection->Keyframes.Sort([](const FCapsuleKeyframeData& A, const FCapsuleKeyframeData& B)
                                   { return A.Time < B.Time; });

    // 标记脏并刷新 Sequencer
    CapsuleSection->MarkPackageDirty();
    Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
}

//#endregion

#undef LOCTEXT_NAMESPACE