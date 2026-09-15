// Copyright (c) 2025 Jared Taylor

#pragma once

/*
 * This contains the same Anim Graph functions used in the Anim Graph Blueprint, but in C++
 * Its purpose is to be conveniently diffed and show any changes to the Anim Graph logic in a more readable way
 * You should NOT include this file and copy what you need out, or reproduce the changes in your Anim Graph Blueprint
 * 
 * SETUP_ functions go in OnBecomeRelevant event
 * UPDATE_ functions go in the OnUpdate event
 * _POSE functions go on the OutputAnimationPose node
 * _ANIM functions go on the SequencePlayer or SequenceEvaluator nodes

UAnimSequence* GetTurnInPlaceAnim(bool bRecovery)
{
	return UTurnInPlaceStatics::GetTurnInPlaceAnimation(TurnData.AnimSet, TurnNodeData, bRecovery);
}

void UMyAnimLayer::Setup_Idle_Pose(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (!CanUpdateAnimGraph()) { return; }

	Main()->TurnNodeData.bHasReachedMaxTurnAngle = false;
	Main()->TurnNodeData.TurnPlayRate = 1.f;
}

void UMyAnimLayer::Setup_TurnInPlace_Pose(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	// This function always occurs prior to the sequence evaluator's OnBecomeRelevant
	// This is because it parses the nodes based on their links so we can be sure these are set prior to the evaluator running its logic

	TurnNodeData.StepSize = TurnData.StepSize;
	TurnNodeData.bIsTurningRight = TurnData.bTurnRight;

	// Reset the entry latch so the next worker pass can re-latch it from the just-entered turn anim's curves.
	// This is what prevents premature TurnInPlace -> TurnRecovery transitions caused by curve staleness.
	bWasTurningThisEntry = false;
}

void UMyAnimLayer::Setup_TurnInPlace_Anim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	// We dumped the previous turn state due to inertialization, so using Set Sequence here will not cause the
	// pre-existing turn animation to snap when repeating this state rapidly

	TurnNodeData.AnimStateTime = 0.f;
	TurnNodeData.bHasReachedMaxTurnAngle = false;
	TurnNodeData.bReachedAnimEnd = false;

	constexpr bool bRecovery = false;
	UAnimSequence* A = GetTurnInPlaceAnim(bRecovery);
	
	const auto& R = NodeType<FSequenceEvaluatorReference>(Node);
	USequenceEvaluatorLibrary::SetSequence(R, A);
	USequenceEvaluatorLibrary::SetExplicitTime(R, 0.f);

	UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceNode(TurnNodeData, TurnData, TurnData.AnimSet);
}

void UMyAnimLayer::Update_TurnInPlace_Anim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	// Even though we use Set Sequence in setup we need to allow changing mid-turn due to potential stance changes
	// updating the current turn animation (e.g. from stand turn to crouch turn)

	constexpr bool bRecovery = false;
	UAnimSequence* A = GetTurnInPlaceAnim(bRecovery);

	const auto& R = NodeType<FSequenceEvaluatorReference>(Node);
	USequenceEvaluatorLibrary::SetSequenceWithInertialBlending(Context, R, A, 0.2f);

	const float Time = UTurnInPlaceStatics::GetUpdatedTurnInPlaceAnimTime_ThreadSafe(A, TurnNodeData.AnimStateTime,
		Context.GetContext()->GetDeltaTime(), TurnNodeData.TurnPlayRate);

	TurnNodeData.AnimStateTime = Time;
	USequenceEvaluatorLibrary::SetExplicitTime(R, Time);

	// Flag when the turn animation has fully played out. This lets the recovery transition fire even if
	// bWasTurningThisEntry never latched (e.g. the turn-yaw-weight curve never registered this entry), which
	// otherwise leaves the character permanently stuck at the end of the turn animation.
	TurnNodeData.bReachedAnimEnd = A && (Time >= A->GetPlayLength() - UE_KINDA_SMALL_NUMBER);

	UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceNode(TurnNodeData, TurnData, TurnData.AnimSet);
}

void UMyAnimLayer::Setup_TurnRecovery_Pose(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	TurnNodeData.bIsRecoveryTurningRight = TurnNodeData.bIsTurningRight;
}

void UMyAnimLayer::Update_TurnRecovery_Anim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	constexpr bool bRecovery = true;
	UAnimSequence* A = GetTurnInPlaceAnim(bRecovery);
	
	USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, NodeType<FSequencePlayerReference>(Node), A, 0.2f);
}

*/