# Starter Guide

## Contract Flow

1. Initialize

### ACTION init()
* app_name: "Telos Works"
* app_version: "v0.1.0"
* initial_admin: "workstester1"

2. Draft Proposal

### ACTION draftprop()
* title: "Proposal 1"
* subtitle: "Worker Proposal 1"
* proposal_name: "worksprop1"
* proposer: "workstester1"
* category: "apps"
* total_requested: "5000.0000 TLOS"
* milestones: 2

3. Launch Proposal

### ACTION launchprop()
* ballot_name: "worksprop1"

4. Vote

### Vote through Telos Decide

5. Close Ballot

### ACTION endprop()
* ballot_name: "worksprop1"