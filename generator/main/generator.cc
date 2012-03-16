// Copyright © 2012, Université catholique de Louvain
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// *  Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// *  Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "generator.hh"

using namespace clang;

clang::ASTContext* context;

void processDeclContext(const DeclContext* ds) {
  for (auto iter = ds->decls_begin(), e = ds->decls_end(); iter != e; ++iter) {
    Decl* decl = *iter;

    if (const SpecDecl* ND = dyn_cast<SpecDecl>(decl)) {
      /* It's a template specialization decl, might be an
       * Interface<T> or an Implementation<T> that we must process. */
      if (ND->getNameAsString() == "Interface") {
        handleInterface(ND);
      } else if (ND->getNameAsString() == "Implementation") {
        handleImplementation(ND);
      }
    } else if (const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(decl)) {
      /* It's a namespace, recurse in it. */
      processDeclContext(nsDecl);
    }
  }
}

int main(int argc, char* argv[]) {
  llvm::IntrusiveRefCntPtr<DiagnosticsEngine> Diags;
  FileSystemOptions FileSystemOpts;

  // Parse source file
  ASTUnit *unit = ASTUnit::LoadFromASTFile(std::string(argv[1]),
                                           Diags, FileSystemOpts,
                                           false, 0, 0, true);

  // Setup printing policy
  // We want the bool type to be printed as "bool"
  context = &(unit->getASTContext());
  PrintingPolicy policy = context->getPrintingPolicy();
  policy.Bool=1;
  context->setPrintingPolicy(policy);

  // Process
  processDeclContext(context->getTranslationUnitDecl());
}
