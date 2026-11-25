// Copyright (c) 2015-2017, Satoshi Tanda. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

/// @file
/// Declares interfaces to VMM initialization functions

#ifndef HYPERPLATFORM_VM_H_
#define HYPERPLATFORM_VM_H_

#include <ntddk.h>

struct ProcessorData;

enum class VmProcessorState { kUnowned, kVmxRoot, kVmxGuest, kRetired };

struct VmProcessorOwner {
  PROCESSOR_NUMBER number;
  VmProcessorState state;
  ProcessorData *data;
  ULONG_PTR native_cr0;
  ULONG_PTR native_cr4;
};

// Serialized by the embedding driver. Offline CPUs remain in the owner roster.
struct VmSession {
  static constexpr ULONG kCapacity = 64;
  VmProcessorOwner processors[kCapacity];
  ULONG processor_count;
};

extern "C" {
////////////////////////////////////////////////////////////////////////////////
//
// macro utilities
//

////////////////////////////////////////////////////////////////////////////////
//
// constants and macros
//

////////////////////////////////////////////////////////////////////////////////
//
// types
//

////////////////////////////////////////////////////////////////////////////////
//
// prototypes
//

/// Virtualizes all processors
/// @return STATUS_SUCCESS on success
///
/// Initializes a VMCS region and virtualizes (ie, enters the VMX non-root
/// operation mode) for each processor. Returns non STATUS_SUCCESS value if any
/// of processors failed to do so. In that case, this function de-virtualize
/// already virtualized processors.
_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS VmInitialization();

/// De-virtualize all processors
_IRQL_requires_max_(PASSIVE_LEVEL) void VmTermination();

/// The session receives partial acquisition on failure and must retire it.
_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS VmStartSession(_Inout_ VmSession *session);

/// Success confirms that the recorded processor returned outside VMX. Its
/// allocation stays owned by the session until aggregate release.
_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
    VmStopSessionProcessor(_Inout_ VmProcessorOwner *processor);

/// Releases the owner tokens only after every acquired processor retired.
_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS VmReleaseSession(_Inout_ VmSession *session);

/// Virtualizes the specified processor
/// @param proc_num   A processor number to virtualize
/// @return STATUS_SUCCESS on success
///
/// The processor 0 must have already been virtualized, or it fails.
_IRQL_requires_max_(PASSIVE_LEVEL) NTSTATUS
    VmHotplugCallback(const PROCESSOR_NUMBER& proc_num);

////////////////////////////////////////////////////////////////////////////////
//
// variables
//

////////////////////////////////////////////////////////////////////////////////
//
// implementations
//

}  // extern "C"

#endif  // HYPERPLATFORM_VM_H_
