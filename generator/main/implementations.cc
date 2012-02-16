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
#include <iostream>

using namespace clang;

struct ImplementationDef {
  ImplementationDef() {
    name = "";
    copiable = false;
    transient = false;
    storage = "";
    base = "Type";
  }

  void makeOutput(llvm::raw_fd_ostream& to);

  std::string name;
  bool copiable;
  bool transient;
  std::string storage;
  std::string base;
};

void handleImplementation(const SpecDecl* ND) {
  const TemplateArgumentList& L=ND->getTemplateArgs();
  assert(L.size()==1);
  assert(L[0].getKind()==TemplateArgument::Type);

  const std::string name =
    dyn_cast<TagType>(L[0].getAsType().getTypePtr())
    ->getDecl()->getNameAsString();

  ImplementationDef definition;
  definition.name = name;

  for(CXXRecordDecl::base_class_const_iterator i=ND->bases_begin(), e=ND->bases_end();
      i!=e;
      ++i) {
    CXXRecordDecl* arg=i->getType()->getAsCXXRecordDecl();
    std::string argLabel=arg->getNameAsString();
    if(argLabel=="Copiable"){
      definition.copiable=true;
    } else if(argLabel=="Transient"){
      definition.transient=true;
    } else if(argLabel=="StoredAs"){
      definition.storage=dyn_cast<SpecDecl>(arg)->getTemplateArgs()[0].getAsType().getAsString(context->getPrintingPolicy());
    } else if(argLabel=="StoredWithArrayOf"){
      definition.storage=dyn_cast<SpecDecl>(arg)->getTemplateArgs()[0].getAsType().getAsString(context->getPrintingPolicy());
      definition.storage="ImplWithArray<Implementation<" + name + ">, " + definition.storage + ">";
    } else if(argLabel=="BasedOn"){
      definition.base=dyn_cast<SpecDecl>(arg)->getTemplateArgs()[0].getAsType().getAsString(context->getPrintingPolicy());
    } else {}
  }

  std::string err;
  llvm::raw_fd_ostream to((name+"-implem.hh").c_str(),err);
  assert(err=="");
  definition.makeOutput(to);
}

void ImplementationDef::makeOutput(llvm::raw_fd_ostream& to) {
  if (storage != "") {
    to << "template <>\n";
    to << "class Storage<" << name << "> {\n";
    to << "public:\n";
    to << "  typedef " << storage << " Type;\n";
    to << "};\n\n";
  }
  to << "class "<< name << ": public " << base << " {\n";
  to << "public:\n";
  to << "  " << name << "() : " << base << "(\"" << name << "\", " << copiable << ", " << transient <<") {}\n";
  to << "  static const " << name << "* const type() {\n";
  to << "    static const " << name << " rawType;\n";
  to << "    return &rawType;\n";
  to << "  }\n";
  to << "};\n\n";
}
