import os
import subprocess

#get VULKAN_SDK DIR
#TODO get environment variable of vulkan sdk
#print(os.environ)

vulkan_dir = os.environ['VULKAN_SDK']
glslang_exe = os.path.join(vulkan_dir, 'bin', 'glslangValidator.exe')

class Shaders:

    def __init__(self, filepath):
        self.filepath = filepath
        if not os.path.exists(filepath):
            raise RuntimeError("no such directory")
        else:
            files = os.listdir(filepath)
            self.vert_shader_files = [v for v in files if v.endswith('.vert')] 
            self.frag_shader_files = [f for f in files if f.endswith('.frag')]
            self.geom_shader_files = [g for g in files if g.endswith('.geom')]
            self.tess_shader_files = [t for t in files if t.endswith('.tess')]

            #vertex shader list is empty
            if not self.vert_shader_files:
                print("WARNING: no vertex shader files found")

            #fragment shader list is empty
            if not self.frag_shader_files:
                print("WARNING: no fragment shader files found")

            #geometry shader list is empty
            if not self.geom_shader_files:
                print("WARNING: no geometry shader files found")

                        #geometry shader list is empty
            if not self.tess_shader_files:
                print("WARNING: no tesselation shader files found")

    def compile(self):

        error = 0

        for v in self.vert_shader_files:
            print("compiling vertex shader file: %s" % v)
            spv_file = v.split(".")
            v_shader = os.path.join(self.filepath, v)
            spv_file = os.path.join(self.filepath, spv_file[0] + ".vspv")

            error = subprocess.call([glslang_exe,"-V", "-H", v_shader,"-o", spv_file])
            
            if error:
                raise RuntimeError("failed to generate vertex shader")

        for f in self.frag_shader_files:
            print("compiling fragment shader file: %s" % f)
            spv_file = f.split(".")
            f_shader = os.path.join(self.filepath, f)
            spv_file = os.path.join(self.filepath, spv_file[0] + ".fspv")
            print("spv file is called: %s " % spv_file)
            error = subprocess.call([glslang_exe,"-V", "-H", f_shader,"-o", spv_file])
            
            if error:
                raise RuntimeError("failed to generate fragment shader")

        for g in self.geom_shader_files:
            print("compiling geometry shader file: %s" % g)
            spv_file = g.split(".")
            g_shader = os.path.join(self.filepath, g)
            spv_file = os.path.join(self.filepath, spv_file[0] + ".gspv")
            error = subprocess.call([glslang_exe,"-V", "-H", g_shader,"-o", spv_file])
        
            if error:
                raise RuntimeError("failed to generate geometry shader")

        for t in self.tess_shader_files:
            print("compiling geometry shader file: %s" % g)
            spv_file = g.split(".")
            t_shader = os.path.join(self.filepath, t)
            spv_file = os.path.join(self.filepath, spv_file[0] + ".gspv")
            error = subprocess.call([glslang_exe,"-V", "-H", t_shader,"-o", spv_file])
        
            if error:
                raise RuntimeError("failed to generate tesselation shader")

        
#get current working directory
cwd = os.getcwd()
vk_shader_dir = os.path.join(cwd)

#now find and compile all the shaders
shaders = Shaders(vk_shader_dir)
shaders.compile()

print("completed")
