/****************************************************************************************\
 *                                                                                      *
 *  CPPDI - Dependency Injection for C++                                                *
 *                                                                                      *
 ****************************************************************************************
 *   MIT License                                                                        *
 *                                                                                      *
 *   Copyright (c) 2023 Luis Condes Diaz                                                *
 *                                                                                      *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy       *
 *   of this software and associated documentation files (the "Software"), to deal      *
 *   in the Software without restriction, including without limitation the rights       *
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell          *
 *   copies of the Software, and to permit persons to whom the Software is              *
 *   furnished to do so, subject to the following conditions:                           *
 *                                                                                      *
 *   The above copyright notice and this permission notice shall be included in all     *
 *   copies or substantial portions of the Software.                                    *
 *                                                                                      *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR         *
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,           *
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE        *
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER             *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,      *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE      *
 *   SOFTWARE.                                                                          *
\****************************************************************************************/
#pragma once
#include <typeinfo>
#ifndef interface
// Portable syntax sugar definition for interface
#define interface struct
#endif
namespace di {
    enum class Instantiation{ singleton, factory };

    interface Binding {
        virtual ~Binding(){}
        virtual const char* GetInterfaceName() = 0;
        virtual const char* GetTypeName() = 0;
        virtual void* Build() = 0;
    };

    template <typename I, class T, class... D> class CtorBind;

    class DiContainer final {
    public:
        DiContainer(){ 
            sealed = false; 
        }
        
        virtual ~DiContainer(){
            for(int i = numBindings-1; i > 0; i--) {
                delete bindings[i];
            }
            delete[] bindings;
        }
        
        template<typename I, class T, class... D> void Bind(Instantiation mode = Instantiation::singleton) {
            if(sealed) {
                // Instance sealed
                return;
            }
            
            Binding* bind = Resolve<I>();
            if(bind != nullptr) {
                // Already binded interface
                return;
            }
            
            bind = Resolve<T>();
            if(bind != nullptr) {
                // Already binded type
                return;
            }
            
            bind = new CtorBind<I,T,D...>(this, mode);
            if(mode == Instantiation::singleton) {
                // Instantiaton mode is singleton, build right now
                bind->Build();
            }
            
            AddBind(bind);
        }
        
        template<typename T> T* Get() {
            Binding* bind = Resolve<T>();
            if(bind == nullptr) {
                // Type/Interface not binded
                return nullptr;
            }
            return (T*)bind->Build();
        }
        
        void Seal() {
            sealed = true;
        }
    private:
        Binding** bindings;
        unsigned int numBindings;
        bool sealed;
        
        template<typename T> Binding* Resolve() {
            for(unsigned int i = 0; i < numBindings; i++) {
                Binding* current = bindings[i];
                if(current->GetInterfaceName() == typeid(T).name() || current->GetTypeName() == typeid(T).name()) {
                    return current;
                }
            }
            return nullptr;
        };
        
        void AddBind(Binding* b){
            Binding** newBinds = new Binding*[numBindings+1];
            if(bindings != nullptr) {
                for(unsigned int i = 0; i < numBindings; i++) {
                    newBinds[i] = bindings[i];
                }
                delete[] bindings;
            }
            newBinds[numBindings] = b;
            bindings = newBinds;
            newBinds = nullptr;
            numBindings++;
        }

        template <typename I, class T, class... D> class CtorBind : public Binding {
        public:
            CtorBind(DiContainer* cont, Instantiation imode){
                mode = imode;
                container = cont;
            }
            
            virtual ~CtorBind(){
                delete instance;
                container = nullptr;
            }
            
            const char* GetInterfaceName() override { return typeid(I).name(); }
            
            const char* GetTypeName() override { return typeid(T).name(); }
            
            void* Build() override {
                if(mode == Instantiation::singleton && instance != nullptr) {
                    // If singleton and the instance exists, return
                    return instance;
                }

                T* obj = new T(container->Get<D>()...);

                if(mode == Instantiation::singleton) {
                    instance = obj;
                }
                
                return obj;
            }
        private:
            Instantiation mode;
            T* instance;
            DiContainer* container;
        };
    };
}
