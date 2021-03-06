//===-- go-llvm-builtins.h - decls for 'BuiltinTable' class ---------------===//
//
// Copyright 2018 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
//
//===----------------------------------------------------------------------===//
//
// Defines BuiltinTable and related classes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVMGOFRONTEND_GO_LLVM_BUILTINTABLE_H
#define LLVMGOFRONTEND_GO_LLVM_BUILTINTABLE_H

#include <unordered_map>

#include "go-llvm-irbuilders.h"

#include "llvm/Analysis/TargetLibraryInfo.h"

class Bfunction;
class Btype;
class TypeManager;

typedef std::vector<Btype*> BuiltinEntryTypeVec;

typedef llvm::Value *(*BuiltinExprMaker)(llvm::SmallVectorImpl<llvm::Value*> &args,
                                         BlockLIRBuilder *builder,
                                         Llvm_backend *be);

// An entry in a table of interesting builtin functions. A given entry
// is either an intrinsic or a libcall builtin.
//
// Intrinsic functions can be generic or target-dependent; they are
// predefined by LLVM, described in various tables (ex:
// .../include/llvm/IR/Intrinsics.td).  Intrinsics can be polymorphic
// (for example, accepting an arg of any float type).
//
// Libcall builtins can also be generic or target-dependent; they
// are identified via enums defined in .../Analysis/TargetLibraryInfo.def).
// These functions are not polymorphic.
//
// Inlined builtins are intrinsics handled by the bridge (instead
// of LLVM). A call to it will turn into an inlined expresssion
// (an LLVM Value and perhaps a sequence of instructions) at
// materialization.

class BuiltinEntry {
 public:
  enum BuiltinFlavor { IntrinsicBuiltin, LibcallBuiltin, InlinedBuiltin };
  static const auto NotInTargetLib = llvm::LibFunc::NumLibFuncs;

  BuiltinFlavor flavor() const { return flavor_; }
  const std::string &name() const { return name_; }
  const std::string &libname() const { return libname_; }
  llvm::Intrinsic::ID intrinsicId() const { return intrinsicId_; }
  llvm::LibFunc libfunc() const { return libfunc_; }
  const BuiltinEntryTypeVec &types() const { return types_; }
  Bfunction *bfunction() const { return bfunction_; }
  void setBfunction(Bfunction *bfunc);
  BuiltinExprMaker exprMaker() const { return exprMaker_; }

  BuiltinEntry(llvm::Intrinsic::ID intrinsicId,
               const std::string &name,
               const std::string &libname,
               const BuiltinEntryTypeVec &ovltypes)
      : name_(name), libname_(libname), flavor_(IntrinsicBuiltin),
        intrinsicId_(intrinsicId), libfunc_(NotInTargetLib),
        bfunction_(nullptr), types_(ovltypes), exprMaker_(nullptr) { }
  BuiltinEntry(llvm::LibFunc libfunc,
               const std::string &name,
               const std::string &libname,
               const BuiltinEntryTypeVec &paramtypes)
      : name_(name), libname_(libname), flavor_(LibcallBuiltin),
        intrinsicId_(), libfunc_(libfunc), bfunction_(nullptr),
        types_(paramtypes), exprMaker_(nullptr) { }
  BuiltinEntry(const std::string &name,
               const std::string &libname,
               const BuiltinEntryTypeVec &paramtypes,
               BuiltinExprMaker exprMaker)
      : name_(name), libname_(libname), flavor_(InlinedBuiltin),
        intrinsicId_(), libfunc_(NotInTargetLib), bfunction_(nullptr),
        types_(paramtypes), exprMaker_(exprMaker) { }
  ~BuiltinEntry();

 private:
  std::string name_;
  std::string libname_;
  BuiltinFlavor flavor_;
  llvm::Intrinsic::ID intrinsicId_;
  llvm::LibFunc libfunc_;
  Bfunction *bfunction_;
  BuiltinEntryTypeVec types_;
  BuiltinExprMaker exprMaker_;
};

// This table contains entries for the builtin functions that may
// be useful/needed for a go backend. The intent here is to generate
// info about the functions at the point where this object is created,
// then create the actual LLVM function lazily at the point where
// there is a need to call the function.
//
// Note: there is support in the table for creating "long double"
// versions of things like the trig builtins, but the CABI machinery
// does not yet handle "long double" (nor does this type exist in Go),
// so it is currently stubbed out (via the "addLongDouble" param below).

class BuiltinTable {
 public:
  BuiltinTable(TypeManager *tman, bool addLongDouble);
  ~BuiltinTable() { }

  void defineAllBuiltins();
  BuiltinEntry *lookup(const std::string &name);

 private:
  void defineSyncFetchAndAddBuiltins();
  void defineIntrinsicBuiltins();
  void defineTrigBuiltins();
  void defineExprBuiltins();

  void defineLibcallBuiltin(const char *libname, const char *name,
                            unsigned libfuncID, ...);

  void defineLibcallBuiltin(const char *libname, const char *name,
                            BuiltinEntryTypeVec &types,
                            unsigned libfuncID);

  void defineIntrinsicBuiltin(const char *name, const char *libname,
                              unsigned intrinsicID, ...);

  void registerIntrinsicBuiltin(const char *name,
                                const char *libname,
                                llvm::Intrinsic::ID intrinsicId,
                                const BuiltinEntryTypeVec &overloadTypes);
  void registerLibCallBuiltin(const char *name,
                              const char *libname,
                              llvm::LibFunc libfunc,
                              const BuiltinEntryTypeVec &paramTypes);
  void registerExprBuiltin(const char *name,
                           const char *libname,
                           const BuiltinEntryTypeVec &paramTypes,
                           BuiltinExprMaker exprMaker);

 private:
  TypeManager *tman_;
  std::unordered_map<std::string, unsigned> tab_;
  std::vector<BuiltinEntry> entries_;
  bool addLongDouble_;
};

#endif // LLVMGOFRONTEND_GO_LLVM_BUILTINTABLE_H
