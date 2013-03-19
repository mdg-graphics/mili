
###################################################################
#
# grizIt_inputHelper.py - To help parse input
#
#     Sara Drakeley
#     LLNL
#     June 29th, 2010
#
#
###################################################################
#  Modifications:
#     S. Drakeley - June 29th, 2010: Created.
#
###################################################################
#

import grizIt_Session as S
session = S.session

##################################################################


class InputParser(object):
    ''' A helper class to handle string arguments
    '''
    def __init__(self):
        pass

    def string_to_list(self, input_string):
        ''' Takes a string such as '[1-2 20 21 30-50]' and uses it to
            populate a list

            If given "all" it turns all returns a list of all materials
    
            *Note* May need to make safe in case people put spaces between hyphens: 1 - 2 vs 1-2
        '''
        output_list = []
        assert type(input_string)==str, 'Argument must be string, list or integer. For string, surround with quotes.'
        mod_str = input_string.strip('[')
        mod_str = mod_str.strip(']')
        mid_list = mod_str.split()
        
        for item in mid_list:
            if '-' in item:
                num_range = item.split('-')
                for k in range(int(num_range[0]),int(num_range[1])+1):
                    output_list.append(k)
            elif item == 'all':
                allMaterials = session.materialNums # a list
                
                for m in allMaterials:
                    d2 = S.DebugMsg(m)
                output_list = allMaterials
            else:
                output_list.append(int(item))
        return output_list

    def stringList_to_list(self, input_list):
        """ ['1'] -> 1 """
        output_list = []
        for j in input_list:
            try:
                output_list.append(int(j))
            except:
                new_mats = self.string_to_list(j)
                for mat in new_mats:
                    output_list.append(mat)
        return output_list
                

    def inputToList(self, materials_list):
        if type(materials_list)==str:
            materials_list = self.string_to_list(materials_list)
        elif type(materials_list)==int:
            materials_list = [materials_list]
        elif type(materials_list)==list:
            materials_list = self.stringList_to_list(materials_list)
        return materials_list





class UpdateEnabled(object):
    def __init__(self):
        self.currentEnabledMats = session.enabled_mats
    def addMaterials(self, materials_list):
        for mat in materials_list:
            if mat not in self.currentEnabledMats:
                self.currentEnabledMats.append(mat)
                self.currentEnabledMats.sort()
        session.updateVar(session.enabled_mats, self.currentEnabledMats)
    def subtractMaterials(self, materials_list):
        for mat in materials_list:
            if mat in self.currentEnabledMats:
                self.currentEnabledMats.remove(mat)
        session.updateVar(session.enabled_mats, self.currentEnabledMats)
        
        


