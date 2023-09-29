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

namespace DI {

    enum class Instantiation{ singleton, factory, provided };

    class Container final {
    private:
        struct Binding {
            virtual ~Binding(){}
            virtual const char* GetTypeName() = 0;
            virtual void* Build() = 0;
        };

        struct TrackedInstance {
            virtual ~TrackedInstance(){}
            virtual const char* GetTypeName() = 0;
            virtual Instantiation GetInstantiation() = 0;
            virtual void* Get() = 0;
        };

        template <class T, class... D> class Ctor : public Binding {
            public:
                Ctor(Container* cont) : container(cont){}
                virtual ~Ctor(){ container = nullptr; }
                
                const char* GetTypeName() override { return typeid(T).name(); }

                void* Build() override { return new T(container->Get<D>()...); }
            private:
                Container* container;
        };

        template <class T> class TypeInstance : public TrackedInstance {
            public:
                TypeInstance(T* obj, DI::Instantiation ins) : instance(obj), mode(ins){}
                virtual ~TypeInstance(){  delete instance; }
                
                const char* GetTypeName() override { return typeid(T).name(); }

                Instantiation GetInstantiation() override { return mode; }

                void* Get() override { return instance; }
            private:
                T* instance;
                Instantiation mode;
        };

        Binding** bindings;
        unsigned int numBindings;
        TrackedInstance** instances;
        unsigned int numInstances;
        bool sealed;

    public:
        Container() : sealed(false), numBindings(0), bindings(nullptr), numInstances(0), instances(0){}
        
        virtual ~Container(){
            for(int i = numBindings-1; i >= 0; i--) {
                delete bindings[i];
            }
            delete[] bindings;
            for(int i = numInstances-1; i >= 0; i--) {
                if(instances[i]->GetInstantiation() != Instantiation::provided) {
                    delete instances[i];
                }
                instances[i] = nullptr;
            }
            delete[] instances;
        }

        void Seal() {
            sealed = true;
        }
        
        template<class T, class... D> void Bind(Instantiation mode = Instantiation::singleton, T* instance = nullptr) {
            if(sealed) { // If instance sealed, return
                return;
            }
            
            Binding* bind = Find<Binding, T>(bindings, numBindings);
            if(bind != nullptr) { // If already binded type, return
                return;
            }
            
            bind = new Ctor<T,D...>(this);
            AddBind(bind);

            if(mode == Instantiation::singleton) { // If instantiation mode is singleton, build and add instance
                AddInstance(new TypeInstance<T>((T*)bind->Build(), DI::Instantiation::singleton));
            }

            if(mode == Instantiation::provided) { // If instantiation mode is provided, add instance
                AddInstance(new TypeInstance<T>(instance, DI::Instantiation::provided));
            }
        }
        
        template<typename T> T* Get() {
            TrackedInstance* instance = Find<TrackedInstance, T>(instances, numInstances);
            if(instance != nullptr) { // If instance found, return it unless its factory based
                if(instance->GetInstantiation() != Instantiation::factory) {
                    return (T*)instance->Get();
                }
            }

            Binding* bind = Find<Binding, T>(bindings, numBindings);
            if(bind == nullptr) { // If type not binded, return nullptr
                return nullptr;
            }

            T* obj = (T*)bind->Build();

            AddInstance(new TypeInstance<T>(obj, DI::Instantiation::factory));

            return obj;
        }
    private:
        template <class E, class T> E* Find(E** arr, unsigned int num) {
            for(unsigned int i = 0; i < num; i++) {
                if(arr[i]->GetTypeName() == typeid(T).name()) {
                    return arr[i];
                }
            }
            return nullptr;
        }
        
        template <class T> void ExpandArray(T*** arr, unsigned int size) {
            T** newArr = new T*[size];
            if(*arr != nullptr) {
                for(unsigned int i = 0; i < size; i++) {
                    newArr[i] = (*arr)[i];
                }
                delete[] *arr;
            }
            *arr = newArr;
            newArr = nullptr;
        }

        void AddBind(Binding* b){
            ExpandArray(&bindings, numBindings+1);
            bindings[numBindings] = b;
            numBindings++;
        }

        void AddInstance(TrackedInstance* tr) {
            ExpandArray(&instances, numInstances+1);
            instances[numInstances] = tr;
            numInstances++;
        }
    };
}
