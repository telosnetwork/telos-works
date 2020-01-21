# Starter Guide

## 1. Initialize

ACTION init()
* app_name: "Telos Works"
* app_version: "v0.1.0"
* initial_admin: "workstester1"

## 2. Draft Proposal

ACTION draftprop()
* title: "Proposal 1"
* description: "Works Proposal 1"
* content: "none"
* proposal_name: "worksprop1"
* proposer: "workstester1"
* category: "apps"
* total_requested: "1000.0000 TLOS"
* milestones: 2

## 3. Start First Milestone

ACTION launchprop()
* proposal_name: "worksprop1"

## 4. Vote

Vote through Telos Decide

## 5. Close Ballot

ACTION closems()
* proposal_name: "worksprop1"

Note that this will render a decision based on the ballot results.

The milestone status will be changed to either passed or failed.

## 6. Submit Report

ACTION submitreport()
* proposal_name: "worksprop1"
* report: "report1"

## 7. Claim Funds

ACTION claimfunds
* proposal_name: "worksprop1"

## 8. Start Next Milestone

ACTION nextms()
* proposal_name: "worksprop1"
* ballot_name: "worksprop2"

### Repeat steps 4 - 8 for every milestone

Note: Failing the first milestone, or two milestones in a row will auto-fail the proposal.
