// #include <libc/bits/stdint.h>

#include "file-utils.hpp"

#include <cassert>
#include <string.h>
#include <dlfcn.h>
#include <vector>

#include <eosio/vm/backend.hpp>
#include <eosio/utils.hpp>
// #include <eosio/eosio.hpp>
#include <native/eosio/crt.hpp>
#include <native/eosio/intrinsics.hpp>

using namespace eosio;
using namespace eosio::cdt;
using namespace eosio::native;

struct my_host_functions {
   static int test(int value) { return value + 42; }
   static int test2(int value) { return value * 42; }
};

std::vector<uint8_t> read_wasm( const std::string& fname ) {
   std::ifstream wasm_file(fname, std::ios::binary);
   assert( wasm_file.is_open() );
   wasm_file.seekg(0, std::ios::end);
   std::vector<uint8_t> wasm; 
   int len = wasm_file.tellg();
   assert( len >= 0 );
   wasm.resize(len);
   wasm_file.seekg(0, std::ios::beg);
   wasm_file.read((char*)wasm.data(), wasm.size());
   wasm_file.close();
   return wasm;
}

void setup_intrinsics() {
   intrinsics::set_intrinsic<intrinsics::require_auth>(
         +[](capi_name nm) {
         });
   intrinsics::set_intrinsic<intrinsics::prints_l>(
      +[](const char* cs, uint32_t l) {
         for (int i=0; i < l; i++)
            printf("%c", cs[i]);
         });
   intrinsics::set_intrinsic<intrinsics::prints>(
      +[](const char* cs) {
         for (int i=0; cs[i] != '\0'; i++)
            printf("%c", cs[i]);
         });
   intrinsics::set_intrinsic<intrinsics::printn>(
      +[](uint64_t nm) {
         std::string s = eosio::name(nm).to_string();
         intrinsics::get_intrinsic<intrinsics::prints_l>()(s.c_str(), s.length());
      });
}

int main(int argc, const char **argv) {
   void    *handle;
   void     (*apply)(uint64_t, uint64_t, uint64_t);
   void     (*initialize)();
   void     (*register_intrinsic)(uint32_t, void*(*)());

   std::cout << "main begin" << std::endl;

   const char* so = "/home/dima/work/cdt/examples/hello/build/tests/libhello_test.so";

   assert( utils::get_file_type(so) == utils::file_type::elf_shared_object );

   /* open the needed object */
   handle = dlopen(so, RTLD_LOCAL | RTLD_LAZY);
   if(handle == NULL)
   {
      printf("Error: %s\n",  dlerror());
      assert(0);
   }

   /* find the address of function and data objects */
   *(void **)(&apply) = dlsym(handle, "apply");
   *(void **)(&initialize) = dlsym(handle, "initialize");
   *(void **)(&register_intrinsic) = dlsym(handle, "register_intrinsic");

   assert(apply);
   assert(initialize);
   assert(register_intrinsic);

   setup_intrinsics();

   printf("registring intrinsics\n");
   auto&& prints_l = *intrinsics::get_intrinsic<intrinsics::prints_l>().target<void (*)(const char*, uint32_t)>();
   assert(prints_l);
   register_intrinsic(intrinsics::prints_l, reinterpret_cast<void *(*)()>(prints_l));
   auto&& prints = *intrinsics::get_intrinsic<intrinsics::prints  >().target<void (*)(const char*)>();
   assert(prints);
   register_intrinsic(intrinsics::prints,   reinterpret_cast<void *(*)()>(prints));
   auto&& printn = *intrinsics::get_intrinsic<intrinsics::printn  >().target<void (*)(uint64_t)>();
   assert(printn);
   register_intrinsic(intrinsics::printn,   reinterpret_cast<void *(*)()>(printn));
   auto&& require_auth = *intrinsics::get_intrinsic<intrinsics::require_auth>().target<void (*)(capi_name)>();
   assert(require_auth);
   register_intrinsic(intrinsics::require_auth,   reinterpret_cast<void *(*)()>(require_auth));
   printf("done\n");

   printf("calling initialize\n");
   initialize();
   printf("done\n");

   printf("calling apply\n");
   apply(string_to_name("test"), string_to_name("test"), string_to_name("hi") );
   printf("done\n");

   using rhf_t = eosio::vm::registered_host_functions<eosio::vm::standalone_function_t>;
   rhf_t::add<&my_host_functions::test>("host", "test");
   rhf_t::add<&my_host_functions::test2>("host", "test2");

   using backend_t = eosio::vm::backend<rhf_t, eosio::vm::interpreter>;

   const char* wasm = "/home/dima/work/cdt/tools/external/eos-vm/tests/host.wasm";

   assert( utils::get_file_type(wasm) == utils::file_type::wasm );

   auto code = read_wasm( wasm );
   eosio::vm::wasm_allocator wa;
   backend_t bkend( code, &wa );

   assert(bkend.call_with_return("env", "test", UINT32_C(5))->to_i32() == 49);
   assert(bkend.call_with_return("env", "test.indirect", UINT32_C(5), UINT32_C(0))->to_i32() == 47);
   assert(bkend.call_with_return("env", "test.indirect", UINT32_C(5), UINT32_C(1))->to_i32() == 210);
   assert(bkend.call_with_return("env", "test.indirect", UINT32_C(5), UINT32_C(2))->to_i32() == 49);
   try {
      bkend.call("env", "test.indirect", UINT32_C(5), UINT32_C(3));
      assert(0);
   } catch (std::exception) {
      assert(1);
   }

   assert(bkend.call_with_return("env", "test.local-call", UINT32_C(5))->to_i32() == 147);

   printf("wasm test successful\n");

   return 0;
}